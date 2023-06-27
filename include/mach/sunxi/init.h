/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MACH_SUNXI_INIT_H
#define MACH_SUNXI_INIT_H

void sun50i_cpu_lowlevel_init(void);

void sun50i_cpu_lowlevel_reset(void);

void sun50i_uart_setup(void);

unsigned long sun50i_a64_ddr3_dram_init(void);
unsigned long sun50i_a64_lpddr3_dram_init(void);

#endif
