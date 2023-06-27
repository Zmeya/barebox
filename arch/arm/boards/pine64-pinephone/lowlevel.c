// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <debug_ll.h>
#include <linux/sizes.h>
#include <linux/bitops.h>
#include <asm/barebox-arm.h>
#include <mach/sunxi/init.h>
#include <mach/sunxi/xload.h>
#include <mach/sunxi/egon.h>
#include <mach/sunxi/rmr_switch.h>
#include <mach/sunxi/sun50i-regs.h>
#include <mach/sunxi/sunxi-pinctrl.h>

#ifdef DEBUG
static void debug_led_rgb(int rgb)
{
       void __iomem *piobase = SUN50I_PIO_BASE_ADDR;
       uint32_t clr, set = 0;
       int r = rgb & 0xff0000;
       int g = rgb & 0x00ff00;
       int b = rgb & 0x0000ff;

       clr = (1 << 19) | (1 << 18) | (1 << 20);
       set |= r ? 1 << 19 : 0;
       set |= g ? 1 << 18 : 0;
       set |= b ? 1 << 20 : 0;

       clrsetbits_le32(piobase + PIO_PD_DATA, clr, set);
}

static void debug_led_init(void)
{
       void __iomem *ccubase = SUN50I_CCU_BASE_ADDR;
       void __iomem *piobase = SUN50I_PIO_BASE_ADDR;

       /* PIO clock enable */
       setbits_le32(ccubase + CCU_BUS_CLK_GATE2, BIT(5));
       /* LED set output */
       clrsetbits_le32(piobase + PIO_PD_CFG2, 0x000fff00, 0x00011100);
}
#else
static void debug_led_rgb(int rgb) {}
static void debug_led_init(void) {}
#endif

// SUN50I_A64_ENTRY_FUNCTION
ENTRY_FUNCTION(start_pine64_pinephone, r0, r1, r2)
{
       extern char __dtb_z_sun50i_a64_pinephone_1_2_start[];
       void *fdt;
       u32 size;

       sunxi_switch_to_aarch64(.text_head_soc_header2, SUN50I_A64_RVBAR_IOMAP);

       debug_led_init();
       debug_led_rgb(0xffff00);

       sun50i_cpu_lowlevel_init();
       sun50i_uart_setup();

       relocate_to_current_adr();
       setup_c();

       /* Skip SDRAM initialization if we run from it */
       if (get_pc() < SUN50I_DRAM_ADDR) {
               size = sun50i_a64_lpddr3_dram_init();
               if (size == 0) {
                       puts_ll("FAIL: dram init\r\n");
                       goto reset;
               }
               puthex_ll(size);
               putc_ll('\r');  putc_ll('\n');
       }

       puts_ll("now booting\r\n");
       fdt = __dtb_z_sun50i_a64_pinephone_1_2_start + get_runtime_offset();
       barebox_arm_entry(SUN50I_DRAM_ADDR, SZ_1G, fdt);

reset:
       debug_led_rgb(0xff0000);
       sun50i_cpu_lowlevel_reset();
}

// SUN50I_A64_ENTRY_FUNCTION
ENTRY_FUNCTION(start_pine64_pinephone_xload, r0, r1, r2)
{

       sunxi_egon_header(.text_head_soc_header0);
       sunxi_switch_to_aarch64(.text_head_soc_header1, SUN50I_A64_RVBAR_IOMAP);

       debug_led_init();
       debug_led_rgb(0xff0000);

       sun50i_cpu_lowlevel_init();
       sun50i_uart_setup();
       debug_led_rgb(0xffff00);

       relocate_to_current_adr();
       setup_c();

       sun50i_a64_lpddr3_dram_init();

       debug_led_rgb(0xff0000);

       sun50i_cpu_lowlevel_reset();
}
