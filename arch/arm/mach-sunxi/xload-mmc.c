// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <asm/cache.h>
#include <linux/bitops.h>
#include <mach/sunxi/sun50i-regs.h>
#include <mach/sunxi/sunxi-pinctrl.h>
#include <mach/sunxi/xload.h>

static void sun50i_mmc0_init(void)
{
       void __iomem *ccu = SUN50I_CCU_BASE_ADDR;
       void __iomem *pio = SUN50I_PIO_BASE_ADDR;

       /* - clock un-gate pinctrl controller */
       setbits_le32(ccu + CCU_BUS_CLK_GATE2, BIT(5));
       /* - set mmc alt-function (2) for pins PF5 to PF0 */
       sunxi_pinmux_set_func(pio, PIO_PF_CFG0, GENMASK(5, 0), 2);

       /* - clock un-gate mmc controller and release reset */
       setbits_le32(ccu + 0x2c0, /* RST_BUS_MMC0 */ BIT(8));
       setbits_le32(ccu + 0x060, /* CLK_BUS_MMC0 */ BIT(8));
       writel(BIT(31), ccu + 0x88); /* MMC0 clock gate */
}

#if 0 /* currently unused */
static void sun50i_mmc2_init(void)
{
       void __iomem *ccu = SUN50I_CCU_BASE_ADDR;
       void __iomem *pio = SUN50I_PIO_BASE_ADDR;

       /* - clock un-gate pinctrl controller */
       setbits_le32(ccu + CCU_BUS_CLK_GATE2, BIT(5));
       /* - set mmc alt-function (3) for pins PC16 to PC5 */
       sunxi_pinmux_set_func(pio, PIO_PC_BASE, GENMASK(16, 5), 3);

       /* - clock un-gate mmc controller and release reset */
       setbits_le32(ccu + 0x2c0, /* RST_BUS_MMC2 */ BIT(10));
       setbits_le32(ccu + 0x060, /* CLK_BUS_MMC2 */ BIT(10));
       writel(BIT(31), ccu + 0x90); /* MMC2 clock gate */
}
#endif

static void sunxi_fat_start_image(struct pbl_bio *bio, void *buf, size_t size)
{
       void (*start)(void) = buf;
       int ret;

       ret = pbl_fat_load(bio, "barebox.bin", buf, size);
       if (ret < 0)
               return;
       sync_caches_for_execution();
       start();
}

void sun50i_mmc0_start_image(void)
{
       struct pbl_bio bio;
       int ret;

       sun50i_mmc0_init();

       ret = sunxi_mmc_bio_init(&bio, SUN50I_MMC0_BASE_ADDR, 24000000, 0);
       if (ret)
               return;
       sunxi_fat_start_image(&bio, IOMEM(SUN50I_DRAM_ADDR), SZ_16M);
}
