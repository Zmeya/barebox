/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MACH_SUNXI_SUN50I_REGS_H
#define MACH_SUNXI_SUN50I_REGS_H

#include <linux/sizes.h>

#define SUN50I_A64_RVBAR_IOMAP 0x017000a0
#define SUN50I_PBL_STACK_TOP   (SUN50I_SRAM_A1_ADDR + SUN50I_SRAM_A1_SIZE)

#define SUN50I_SRAM_A1_ADDR    0x00010000
#define SUN50I_SRAM_A1_SIZE    SZ_32K
#define SUN50I_SRAM_A2_ADDR    0x00044000
#define SUN50I_SRAM_A2_SIZE    SZ_64K
#define SUN50I_SRAM_C_ADDR     0x00018000
/* advertised as 160K, but only ~108K can be used */
#define SUN50I_SRAM_C_SIZE     (108 * SZ_1K)

#define SUN50I_DRAM_ADDR       0x40000000

#define SUN50I_CCU_BASE_ADDR   IOMEM(0x01c20000)
#define SUN50I_PIO_BASE_ADDR   IOMEM(0x01c20800)
#define SUN50I_MMC0_BASE_ADDR  IOMEM(0x01c0f000)
#define SUN50I_MMC1_BASE_ADDR  IOMEM(0x01c10000)
#define SUN50I_MMC2_BASE_ADDR  IOMEM(0x01c11000)
#define SUN50I_TIMER_BASE_ADDR IOMEM(0x01c20c00)

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

#endif
