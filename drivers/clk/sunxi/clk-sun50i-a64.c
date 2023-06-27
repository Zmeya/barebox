// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2022 Jules Maselbas

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <linux/clkdev.h>
#include <linux/err.h>

#define MHZ (1000 * 1000UL)

#include "clk-sun50i-a64.h"

#define CCU_PLL_CPUX           0x00
#define CCU_PLL_PERIPH0                0x28
#define CCU_CPUX_AXI_CFG       0x50
#define CCU_AHB1_APB1_CFG      0x54
#define CCU_APB2_CFG           0x58
#define CCU_AHB2_CFG           0x5c
#define CCU_BUS_CLK_GATE0      0x60
#define CCU_BUS_CLK_GATE1      0x64
#define CCU_BUS_CLK_GATE2      0x68
#define CCU_BUS_CLK_GATE3      0x6c
#define CCU_CE_CLK             0x9c
#define CCU_MBUS_CLK           0x15c
#define CCU_BUS_SOFT_RST0      0x2c0
#define CCU_BUS_SOFT_RST4      0x2d8
#define CCU_PLL_LOCK_CTRL      0x320

static struct clk *clks[CLK_NUMBER];
static struct clk_onecell_data clk_data = {
       .clks = clks,
       .clk_num = ARRAY_SIZE(clks),
};

static inline struct clk *
sunxi_clk_gate(const char *name, const char *parent, void __iomem *reg, u8 shift)
{
       return clk_gate(name, parent, reg, shift, 0, 0);
}

static inline struct clk *
sunxi_clk_mux(const char *name, const char * const *parents, u8 num_parents,
             void __iomem *reg, u8 shift, u8 width)
{
       return clk_mux(name, 0, reg, shift, width, parents, num_parents, 0);
}

static inline struct clk *
sunxi_clk_div(const char *name, const char *parent, struct clk_div_table *table,
             void __iomem *reg, u8 shift, u8 width)
{
       return clk_divider_table(name, parent, CLK_SET_RATE_PARENT, reg, shift,
                                width, table, 0);
}

static struct clk_div_table div_apb1[] = {
       { .val = 0, .div = 2 },
       { .val = 1, .div = 2 },
       { .val = 2, .div = 4 },
       { .val = 3, .div = 8 },
       { /* Sentinel */ },
};

static struct clk_div_table div_N[] = {
       { .val = 0, .div = 1 },
       { .val = 1, .div = 2 },
       { .val = 2, .div = 4 },
       { .val = 3, .div = 8 },
       { /* Sentinel */ },
};

static const char *sel_cpux[] = {
       "osc32k",
       "osc24M",
       "pll-cpux",
};

static const char *sel_ahb1[] = {
       "osc32k",
       "osc24M",
       "axi",
       "pll-periph0",
};

static const char *sel_apb2[] = {
       "osc32k",
       "osc24M",
       "pll-periph0-2x",
       "pll-periph0-2x",
};

static const char *sel_ahb2[] = {
       "ahb1",
       "pll-periph0",
};

static const char *sel_mmc[] = {
       "osc24M",
       "pll-periph0-2x",
       "pll-periph1-2x",
};

static void sun50i_a64_resets_init(void __iomem *regs)
{
       u32 rst;

       rst = 0 |
               /* RST_USB_PHY0 */ BIT(0) |
               /* RST_USB_PHY1 */ BIT(1) |
               /* RST_USB_HSIC */ BIT(2);
       writel(rst, regs + 0x0cc);

       rst = 0 |
               /* RST_BUS_MIPI_DSI */ BIT(1) |
               /* RST_BUS_CE   */ BIT(5) |
               /* RST_BUS_DMA  */ BIT(6) |
               /* RST_BUS_MMC0 */ BIT(8) |
               /* RST_BUS_MMC1 */ BIT(9) |
               /* RST_BUS_MMC2 */ BIT(10) |
               /* RST_BUS_NAND */ BIT(13) |
               /* RST_BUS_DRAM */ BIT(14) |
               /* RST_BUS_EMAC */ BIT(17) |
               /* RST_BUS_TS   */ BIT(18) |
               /* RST_BUS_HSTIMER */ BIT(19) |
               /* RST_BUS_SPI0  */ BIT(20) |
               /* RST_BUS_SPI1  */ BIT(21) |
               /* RST_BUS_OTG   */ BIT(23) |
               /* RST_BUS_EHCI0 */ BIT(24) |
               /* RST_BUS_EHCI1 */ BIT(25) |
               /* RST_BUS_OHCI0 */ BIT(28) |
               /* RST_BUS_OHCI1 */ BIT(29);
       writel(rst, regs + 0x2c0);

       rst = 0 |
               /* RST_BUS_VE    */ BIT(0) |
               /* RST_BUS_TCON0 */ BIT(3) |
               /* RST_BUS_TCON1 */ BIT(4) |
               /* RST_BUS_DEINTERLACE */ BIT(5) |
               /* RST_BUS_CSI   */ BIT(8) |
               /* RST_BUS_HDMI0 */ BIT(10) |
               /* RST_BUS_HDMI1 */ BIT(11) |
               /* RST_BUS_DE    */ BIT(12) |
               /* RST_BUS_GPU   */ BIT(20) |
               /* RST_BUS_MSGBOX   */ BIT(21) |
               /* RST_BUS_SPINLOCK */ BIT(22) |
               /* RST_BUS_DBG */ BIT(31);
       writel(rst, regs + 0x2c4);

       rst = /* RST_BUS_LVDS */ BIT(0);
       writel(rst, regs + 0x2c8);

       rst = 0 |
               /* RST_BUS_CODEC */ BIT(0) |
               /* RST_BUS_SPDIF */ BIT(1) |
               /* RST_BUS_THS   */ BIT(8) |
               /* RST_BUS_I2S0  */ BIT(12) |
               /* RST_BUS_I2S1  */ BIT(13) |
               /* RST_BUS_I2S2  */ BIT(14);
       writel(rst, regs + 0x2d0);

       rst = 0 |
               /* RST_BUS_I2C0 */ BIT(0) |
               /* RST_BUS_I2C1 */ BIT(1) |
               /* RST_BUS_I2C2 */ BIT(2) |
               /* RST_BUS_SCR  */ BIT(5) |
               /* RST_BUS_UART0 */ BIT(16) |
               /* RST_BUS_UART1 */ BIT(17) |
               /* RST_BUS_UART2 */ BIT(18) |
               /* RST_BUS_UART3 */ BIT(19) |
               /* RST_BUS_UART4 */ BIT(20);
       writel(rst, regs + 0x2d8);
}

static int
sunxi_clk_set_pll(void __iomem *reg, u32 src, u32 freq)
{
       /* NOTE: using u32, max freq is 4GHz
        * out freq: src * N * K
        * factor N: [1->32]
        * factor K: [1->4]
        * from the manual: give priority to the choice of K >= 2
        */
       u32 mul = freq / src; /* target multiplier (N * K) */
       u32 k, n;
       u32 cfg = BIT(31); /* ENABLE */

       for (k = 4; k > 1; k--) {
               if ((mul % k) == 0)
                       break;
       }
       n = mul / k;

       cfg |= (k - 1) << 4;
       cfg |= (n - 1) << 8;

       writel(cfg, reg);
       return wait_on_timeout(1 * MSECOND, readl(reg) & BIT(28));
}

static void sun50i_a64_clocks_init(struct device_node *np, void __iomem *regs)
{
       sun50i_a64_resets_init(regs);

       /* set pll-cpu to 1.2GHz */
       if (sunxi_clk_set_pll(regs + CCU_PLL_CPUX, 24 * MHZ, 1200 * MHZ))
               pr_err("fail to lock pll-cpu @ 1.2GHz\n");
       clks[CLK_PLL_CPUX] = clk_fixed("pll-cpux", 1200 * MHZ);

       /* switch cpu clock source: cpux_src: 1=24mhz 2=PLL_CPUX */
       clks[CLK_CPUX] = sunxi_clk_mux("cpux", sel_cpux, ARRAY_SIZE(sel_cpux), regs + CCU_CPUX_AXI_CFG, 16, 2);
       writel(0x20001, regs + CCU_CPUX_AXI_CFG); /* select pll-cpu */
       udelay(1); /* wait 8 cycles */

       /* set pll-periph0-2x to 1.2GHz, as recommended */
       if (sunxi_clk_set_pll(regs + CCU_PLL_PERIPH0, 24 * MHZ, 2 * 600 * MHZ))
               pr_err("fail to lock pll-periph @ 1.2GHz\n");

       clks[CLK_PLL_PERIPH0]    = clk_fixed("pll-periph0", 600 * MHZ);
       clks[CLK_PLL_PERIPH0_2X] = clk_fixed_factor("pll-periph0-2x", "pll-periph0", 2, 1, 0);

       clks[CLK_AHB1] = sunxi_clk_mux("ahb1", sel_ahb1, ARRAY_SIZE(sel_ahb1), regs + 0x054, 12, 2);
       clks[CLK_AHB2] = sunxi_clk_mux("ahb2", sel_ahb2, ARRAY_SIZE(sel_ahb2), regs + 0x05c, 0, 1);

       clks[CLK_APB1] = sunxi_clk_div("apb1", "ahb1", div_apb1, regs + 0x054, 8, 2);
       clks[CLK_APB2] = sunxi_clk_mux("apb2", sel_apb2, ARRAY_SIZE(sel_apb2), regs + 0x058, 24, 2);

       clks[CLK_BUS_MIPI_DSI] = sunxi_clk_gate("bus-mipi-dsi","ahb1",regs + 0x060, 1);
       clks[CLK_BUS_CE]    = sunxi_clk_gate("bus-ce",    "ahb1", regs + 0x060, 5);
       clks[CLK_BUS_DMA]   = sunxi_clk_gate("bus-dma",   "ahb1", regs + 0x060, 6);
       clks[CLK_BUS_MMC0]  = sunxi_clk_gate("bus-mmc0",  "ahb1", regs + 0x060, 8);
       clks[CLK_BUS_MMC1]  = sunxi_clk_gate("bus-mmc1",  "ahb1", regs + 0x060, 9);
       clks[CLK_BUS_MMC2]  = sunxi_clk_gate("bus-mmc2",  "ahb1", regs + 0x060, 10);
       clks[CLK_BUS_NAND]  = sunxi_clk_gate("bus-nand",  "ahb1", regs + 0x060, 13);
       clks[CLK_BUS_DRAM]  = sunxi_clk_gate("bus-dram",  "ahb1", regs + 0x060, 14);
       clks[CLK_BUS_EMAC]  = sunxi_clk_gate("bus-emac",  "ahb2", regs + 0x060, 17);
       clks[CLK_BUS_TS]    = sunxi_clk_gate("bus-ts",    "ahb1", regs + 0x060, 18);
       clks[CLK_BUS_HSTIMER] = sunxi_clk_gate("bus-hstimer", "ahb1", regs + 0x060, 19);
       clks[CLK_BUS_SPI0]  = sunxi_clk_gate("bus-spi0",  "ahb1", regs + 0x060,  20);
       clks[CLK_BUS_SPI1]  = sunxi_clk_gate("bus-spi1",  "ahb1", regs + 0x060,  21);
       clks[CLK_BUS_OTG]   = sunxi_clk_gate("bus-otg",   "ahb1", regs + 0x060,  23);
       clks[CLK_BUS_EHCI0] = sunxi_clk_gate("bus-ehci0", "ahb1", regs + 0x060, 24);
       clks[CLK_BUS_EHCI1] = sunxi_clk_gate("bus-ehci1", "ahb2", regs + 0x060, 25);
       clks[CLK_BUS_OHCI0] = sunxi_clk_gate("bus-ohci0", "ahb1", regs + 0x060, 28);
       clks[CLK_BUS_OHCI1] = sunxi_clk_gate("bus-ohci1", "ahb2", regs + 0x060, 29);

       clks[CLK_BUS_CODEC] = sunxi_clk_gate("bus-codec", "apb1", regs + 0x068, 0);
       clks[CLK_BUS_SPDIF] = sunxi_clk_gate("bus-spdif", "apb1", regs + 0x068, 1);
       clks[CLK_BUS_PIO]   = sunxi_clk_gate("bus-pio",   "apb1", regs + 0x068, 5);
       clks[CLK_BUS_THS]   = sunxi_clk_gate("bus-ths",   "apb1", regs + 0x068, 8);
       clks[CLK_BUS_I2S0]  = sunxi_clk_gate("bus-i2s0",  "apb1", regs + 0x068, 12);
       clks[CLK_BUS_I2S1]  = sunxi_clk_gate("bus-i2s1",  "apb1", regs + 0x068, 13);
       clks[CLK_BUS_I2S2]  = sunxi_clk_gate("bus-i2s2",  "apb1", regs + 0x068, 14);

       clks[CLK_BUS_UART0] = sunxi_clk_gate("bus-uart0", "apb2", regs + 0x06c, 16);
       clks[CLK_BUS_UART1] = sunxi_clk_gate("bus-uart1", "apb2", regs + 0x06c, 17);
       clks[CLK_BUS_UART2] = sunxi_clk_gate("bus-uart2", "apb2", regs + 0x06c, 18);
       clks[CLK_BUS_UART3] = sunxi_clk_gate("bus-uart3", "apb2", regs + 0x06c, 19);
       clks[CLK_BUS_UART4] = sunxi_clk_gate("bus-uart4", "apb2", regs + 0x06c, 20);

       clks[CLK_MMC0] = clk_register_composite(
               "mmc0", sel_mmc, ARRAY_SIZE(sel_mmc),
               sunxi_clk_mux("mmc0-mux", sel_mmc, ARRAY_SIZE(sel_mmc), regs + 0x088, 24, 2),
               sunxi_clk_div("mmc0-div-n", "mmc0-gate", div_N, regs + 0x088, 16, 2),
               sunxi_clk_gate("mmc0-gate", "mmc0-mux", regs + 0x088, 31),
               0);

       clks[CLK_MMC1] = clk_register_composite(
               "mmc1", sel_mmc, ARRAY_SIZE(sel_mmc),
               sunxi_clk_mux("mmc1-mux", sel_mmc, ARRAY_SIZE(sel_mmc), regs + 0x08c, 24, 2),
               sunxi_clk_div("mmc1-div-n", "mmc1-gate", div_N, regs + 0x08c, 16, 2),
               sunxi_clk_gate("mmc1-gate", "mmc1-mux", regs + 0x08c, 31),
               0);

       clks[CLK_MMC2] = clk_register_composite(
               "mmc2", sel_mmc, ARRAY_SIZE(sel_mmc),
               sunxi_clk_mux("mmc2-mux", sel_mmc, ARRAY_SIZE(sel_mmc), regs + 0x090, 24, 2),
               sunxi_clk_div("mmc2-div-n", "mmc2-gate", div_N, regs + 0x090, 16, 2),
               sunxi_clk_gate("mmc2-gate", "mmc2-mux", regs + 0x090, 31),
               0);

       /* generic set_rate doesn't support switching parent,
        * let's do it here for now */
       clk_set_parent(clks[CLK_MMC0], clks[CLK_PLL_PERIPH0_2X]);
       clk_set_parent(clks[CLK_MMC2], clks[CLK_PLL_PERIPH0_2X]);
}

static int sun50i_a64_ccu_probe(struct device *dev)
{
       struct resource *iores;

       iores = dev_request_mem_resource(dev, 0);
       if (IS_ERR(iores))
               return PTR_ERR(iores);

       sun50i_a64_clocks_init(dev->of_node, IOMEM(iores->start));
       return of_clk_add_provider(dev->of_node, of_clk_src_onecell_get, &clk_data);
}

static __maybe_unused struct of_device_id sun50i_a64_ccu_dt_ids[] = {
       {
               .compatible = "allwinner,sun50i-a64-ccu",
       }, {
               /* sentinel */
       }
};

static struct driver sun50i_a64_ccu_driver = {
       .probe  = sun50i_a64_ccu_probe,
       .name   = "sun50i-a64-ccu",
       .of_compatible = DRV_OF_COMPAT(sun50i_a64_ccu_dt_ids),
};
postcore_platform_driver(sun50i_a64_ccu_driver);
