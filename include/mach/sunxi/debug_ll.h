/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_DEBUG_LL_H__
#define __MACH_DEBUG_LL_H__

#include <io.h>

#define DEBUG_LL_UART_ADDR IOMEM(CONFIG_SUNXI_DEBUG_LL_UART_BASE)

static inline uint8_t debug_ll_read_reg(int reg)
{
       return readl(DEBUG_LL_UART_ADDR + reg * sizeof(uint32_t));
}

static inline void debug_ll_write_reg(int reg, uint8_t val)
{
       writel(val, DEBUG_LL_UART_ADDR + reg * sizeof(uint32_t));
}

#include <debug_ll/ns16550.h>

static inline void debug_ll_init(void)
{
       uint16_t divisor;

       divisor = debug_ll_ns16550_calc_divisor(24 * 1000 * 1000);
       debug_ll_ns16550_init(divisor);
}

static inline void PUTC_LL(int c)
{
       debug_ll_ns16550_putc(c);
}

#endif
