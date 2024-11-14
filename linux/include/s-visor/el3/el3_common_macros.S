/*
 *Description: trusted-firmware-a el3_common_macro.S
 *Date: 2024-11-11 18:50:13
 */

#ifndef __SVISOR_EL3_COMMON_MACROS_S__
#define __SVISOR_EL3_COMMON_MACROS_S__

#include <s-visor/arch/arm64/arch.h>

	.macro	apply_at_speculative_wa
	.endm

	.macro	restore_ptw_el1_sys_regs
	.endm

	.macro el3_switch_sp
		/*
		 * Only used in n-visor call "smc"
		 * x6 has pointed to sp_el3, see s-visor/el3/smc.c
		 */
		mov x5, sp
		msr spsel, #MODE_SP_ELX
		mov sp, x6
	.endm

	.macro el3_enable_sp
		/*
		 * Only used in s-visor call "smc"
		 */
		msr spsel, #MODE_SP_ELX
	.endm

	.macro el3_restore_sp
		/*
		 * x5 has stored the original stack value.
		 * Use this macro after restore_gp_pmcr_pauth_regs()
		 * is called
		 */
		mov sp, x5
	.endm

#endif