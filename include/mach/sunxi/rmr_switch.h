/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef MACH_SUNXI_RMR_SWITCH_H
#define MACH_SUNXI_RMR_SWITCH_H

/*
 * ARMv8 RMR reset sequence on Allwinner SoCs.
 *
 * All 64-bit capable Allwinner SoCs reset in AArch32 (and continue to
 * execute the Boot ROM in this state), so we need to switch to AArch64
 * at some point.
 * Section G6.2.133 of the ARMv8 ARM describes the Reset Management Register
 * (RMR), which triggers a warm-reset of a core and can request to switch
 * into a different execution state (AArch32 or AArch64).
 * The address at which execution starts after the reset is held in the
 * RVBAR system register, which is architecturally read-only.
 * Allwinner provides a writable alias of this register in MMIO space, so
 * we can easily set the start address of AArch64 code.
 * This code below switches to AArch64 and starts execution at the specified
 * start address.
 *
 * This file has been adapted from U-Boot code sources:
 *  - arch/arm/mach-sunxi/rmr_switch.S
 *  - arch/arm/include/asm/arch-sunxi/boot0.h.
 *
 * The aarch32 assembly has already been assembled and are inserted verbatime
 * as .word statements (the asm source is commented for each statement).
 *
 */
#define SUNXI_RMR_SWITCH_CODE(rvbar_addr) {                   \
       0xeb000000, /* bl   .+8             subs x0, x0, x0 */ \
       0x1400000c, /* .word 0x1400000c     b end           */ \
       0xe59f0020, /* ldr  r0, [pc, #32] ; rvbar           */ \
       0xe580e000, /* str  lr, [r0]                        */ \
       0xf57ff04f, /* dsb  sy                              */ \
       0xf57ff06f, /* isb  syo                             */ \
       0xee1c0f50, /* mrc  15, 0, r0, cr12, cr0, {2}       */ \
       0xe3800003, /* orr  r0, r0, #3                      */ \
       0xee0c0f50, /* mcr  15, 0, r0, cr12, cr0, {2}       */ \
       0xf57ff06f, /* isb  sy                              */ \
       0xe320f003, /* 1b: wfi                              */ \
       0xeafffffd, /* b    1b                              */ \
       rvbar_addr, \
       0xd503201f, 0xd503201f, 0xd503201f } /* nop * 3 */
#define sunxi_switch_to_aarch64(section, rvbar_addr) {                 \
               __section(section) static const u32 rmr_switch[] =      \
                       SUNXI_RMR_SWITCH_CODE(rvbar_addr);              \
               __keep_symbolref(rmr_switch);                           \
       }

#endif
