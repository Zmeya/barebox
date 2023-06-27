// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <debug_ll.h>
#include <linux/sizes.h>
#include <linux/bitops.h>
#include <mach/sunxi/barebox-arm.h>
#include <mach/sunxi/init.h>
#include <mach/sunxi/xload.h>
#include <mach/sunxi/egon.h>
#include <mach/sunxi/rmr_switch.h>
#include <mach/sunxi/sun50i-regs.h>
#include <mach/sunxi/sunxi-pinctrl.h>

SUN50I_A64_ENTRY_FUNCTION(start_pine64_pine64, r0, r1, r2)
{
       extern char __dtb_z_sun50i_a64_pine64_plus_start[];
       void *fdt;
       u32 size;

       sunxi_switch_to_aarch64(.text_head_soc_header2, SUN50I_A64_RVBAR_IOMAP);

       sun50i_cpu_lowlevel_init();
       sun50i_uart_setup();

       relocate_to_current_adr();
       setup_c();

       /* Skip SDRAM initialization if we run from it */
       if (get_pc() < SUN50I_DRAM_ADDR) {
               size = sun50i_a64_ddr3_dram_init();
               if (size == 0) {
                       puts_ll("FAIL: dram init\r\n");
                       goto reset;
               }
               puthex_ll(size);
               putc_ll('\r');  putc_ll('\n');
       }

       puts_ll("now booting\r\n");
       fdt = __dtb_z_sun50i_a64_pine64_plus_start + get_runtime_offset();
       barebox_arm_entry(SUN50I_DRAM_ADDR, SZ_1G, fdt);

reset:
       sun50i_cpu_lowlevel_reset();
}

SUN50I_A64_ENTRY_FUNCTION(start_pine64_pine64_xload, r0, r1, r2)
{
       u32 size;

       sunxi_egon_header(.text_head_soc_header0);
       sunxi_switch_to_aarch64(.text_head_soc_header1, SUN50I_A64_RVBAR_IOMAP);

       sun50i_cpu_lowlevel_init();
       sun50i_uart_setup();

       relocate_to_current_adr();
       setup_c();

       size = sun50i_a64_ddr3_dram_init();
       if (size == 0) {
               puts_ll("FAIL: dram init\r\n");
               goto reset;
       }

       /* now let's boot from sd */
       sun50i_mmc0_start_image();
reset:
       sun50i_cpu_lowlevel_reset();
}
