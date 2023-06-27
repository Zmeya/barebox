#define CONFIG_MACH_SUN50I y
#define CONFIG_SUNXI_DRAM_DW y
#define CONFIG_SUNXI_DRAM_DW_32BIT y
#define CONFIG_SUNXI_DRAM_LPDDR3 y
#define CONFIG_SYS_SDRAM_BASE 0x40000000

#define sunxi_dram_init sun50i_a64_lpddr3_dram_init

#include "sun50i-sdram.c"
