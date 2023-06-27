/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
// #include <asm/errata.h> TODO: errata 843419 ?
#include <io.h>
#include <debug_ll.h>
#include <mach/sunxi/init.h>
#include <mach/sunxi/sun50i-regs.h>
#include <mach/sunxi/sunxi-pinctrl.h>

static void sunxi_ccu_init(void __iomem *ccu)
{
       /* running from osc24 */
       set_cntfrq(24 * 1000 * 1000);
       /* APB2 = 24MHz (UART, I2C) */
       writel((1 << 24 /* src: 1=osc24 */) |
              (0 << 16 /* pre_div (N): 0=/1 1=/2 2=/4 3=/8 */) |
              (0 << 0) /* M-1 */,
              ccu + CCU_APB2_CFG);
}

void sun50i_cpu_lowlevel_init(void)
{
       arm_cpu_lowlevel_init();
       sunxi_ccu_init(SUN50I_CCU_BASE_ADDR);
}

void sun50i_cpu_lowlevel_reset(void)
{
       void __iomem *reg = SUN50I_TIMER_BASE_ADDR;
       /* Set the watchdog for its shortest interval (.5s) and wait */
       writel(1, reg + 0xB4); /* reset whole system */
       writel(1, reg + 0xB8); /* enable */
       writel((0xa57 << 1) | 1, reg + 0xB0); /* restart */
       __hang();
}

void sun50i_uart_setup(void)
{
       void __iomem *ccu = SUN50I_CCU_BASE_ADDR;
       void __iomem *pio = SUN50I_PIO_BASE_ADDR;

       /* PIO clock enable */
       setbits_le32(ccu + CCU_BUS_CLK_GATE2, BIT(5));
       /* UART0 clock enable */
       setbits_le32(ccu + CCU_BUS_CLK_GATE3, BIT(16));
       /* UART0 release reset */
       setbits_le32(ccu + CCU_BUS_SOFT_RST4, BIT(16));

       /* UART0 pinmux (PB8 + PB9) */
       sunxi_pinmux_set_func(pio, PIO_PB_CFG0, GENMASK(9, 8), 4);

       debug_ll_init();
       putc_ll('>');
}
