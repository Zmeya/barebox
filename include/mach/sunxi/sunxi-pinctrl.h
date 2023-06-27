/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_SUNXI_PINCTRL_H
#define __MACH_SUNXI_PINCTRL_H

/* low-level gpio "pio" defines to be used in PBL early init,
 * pio aka "allwinner,sun8i-h3-pinctrl" */

#define PIO_Pn_CFG0(n) ((n)+0x0)
#define PIO_Pn_CFG1(n) ((n)+0x4)
#define PIO_Pn_CFG2(n) ((n)+0x8)
#define PIO_Pn_CFG3(n) ((n)+0xc)

#define PIO_PB_BASE (0x24*1)
#define PIO_PC_BASE (0x24*2)
#define PIO_PD_BASE (0x24*3)
#define PIO_PE_BASE (0x24*4)
#define PIO_PF_BASE (0x24*5)
#define PIO_PG_BASE (0x24*6)
#define PIO_PH_BASE (0x24*7)

/* PORT B */
#define PIO_PB_CFG0 PIO_Pn_CFG0(PIO_PB_BASE)
#define PIO_PB_CFG1 PIO_Pn_CFG1(PIO_PB_BASE)
/* PORT C */
#define PIO_PC_CFG0 PIO_Pn_CFG0(PIO_PC_BASE)
#define PIO_PC_CFG1 PIO_Pn_CFG1(PIO_PC_BASE)
#define PIO_PC_CFG2 PIO_Pn_CFG2(PIO_PC_BASE)
/* PORT D */
#define PIO_PD_CFG0 PIO_Pn_CFG0(PIO_PD_BASE)
#define PIO_PD_CFG1 PIO_Pn_CFG1(PIO_PD_BASE)
#define PIO_PD_CFG2 PIO_Pn_CFG2(PIO_PD_BASE)
#define PIO_PD_CFG3 PIO_Pn_CFG3(PIO_PD_BASE)
/* PORT E */
#define PIO_PE_CFG0 PIO_Pn_CFG0(PIO_PE_BASE)
#define PIO_PE_CFG1 PIO_Pn_CFG1(PIO_PE_BASE)
#define PIO_PE_CFG2 PIO_Pn_CFG2(PIO_PE_BASE)
/* PORT F */
#define PIO_PF_CFG0 PIO_Pn_CFG0(PIO_PF_BASE)
/* PORT G */
#define PIO_PG_CFG0 PIO_Pn_CFG0(PIO_PG_BASE)
#define PIO_PG_CFG1 PIO_Pn_CFG1(PIO_PG_BASE)
/* PORT H */
#define PIO_PH_CFG0 PIO_Pn_CFG0(PIO_PH_BASE)
#define PIO_PH_CFG1 PIO_Pn_CFG1(PIO_PH_BASE)

#define PIO_PA_PULL1 0x20
#define PIO_PB_PULL0 0x40
#define PIO_PD_DATA 0x7c

/* static inline so this function can be optimized to a single "call" to clrsetbits */
static inline void sunxi_pinmux_set_func_port(void __iomem *pio, u32 port, u32 pins, u8 func)
{
       u32 i, msk = 0, cfg = 0;

       if (!(pins & 0xff))
               return;

       for (i = 0; i < 8; i++) {
               if (pins & (1 << i)) {
                       cfg |= func << (i * 4);
                       msk |= 0xf << (i * 4);
               }
       }
       clrsetbits_le32(pio + port, msk, cfg);
}

static inline void sunxi_pinmux_set_func(void __iomem *pio, u32 port, u32 pins, u8 func)
{
       sunxi_pinmux_set_func_port(pio, PIO_Pn_CFG0(port), pins, func);
       sunxi_pinmux_set_func_port(pio, PIO_Pn_CFG1(port), pins >> 8, func);
       sunxi_pinmux_set_func_port(pio, PIO_Pn_CFG2(port), pins >> 16, func);
       sunxi_pinmux_set_func_port(pio, PIO_Pn_CFG3(port), pins >> 24, func);
}

#endif
