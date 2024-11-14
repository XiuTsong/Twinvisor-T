/*
 * @Date: 2024-11-08 18:46:59
 */

#ifndef __SVISOR_MISC_H__
#define __SVISOR_MISC_H__

#include <s-visor/common/asm.h>
#include <s-visor/arch/arm64/arch.h>

#ifdef __ASSEMBLER__

.macro __get_core_id reg0, reg1, reg2
	mrs	\reg0, mpidr_el1
	tst	\reg0, #MPIDR_MT_MASK
	lsl	\reg2, \reg0, #MPIDR_AFFINITY_BITS
	csel	\reg2, \reg2, \reg0, eq
	ubfx	\reg0, \reg2, #MPIDR_AFF1_SHIFT, #MPIDR_AFFINITY_BITS
	ubfx	\reg1, \reg2, #MPIDR_AFF2_SHIFT, #MPIDR_AFFINITY_BITS
	add	\reg0, \reg0, \reg1, LSL #2
        /* core_id == \ref0 */
.endm

#endif

#endif
