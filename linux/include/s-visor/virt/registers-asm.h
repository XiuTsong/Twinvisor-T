/*
 * Created on 2024/11/05
 */

#ifndef __SVISOR_REGISTERS_ASM_H__
#define __SVISOR_REGISTERS_ASM_H__

#include <asm/asm-offsets.h>
#include <s-visor/common/asm.h>

.macro save_all_gp_regs ctxt
	stp	x2, x3,   [\ctxt, #CPU_XREG_OFFSET(2)]
	stp	x4, x5,   [\ctxt, #CPU_XREG_OFFSET(4)]
	stp	x6, x7,   [\ctxt, #CPU_XREG_OFFSET(6)]
	stp	x8, x9,   [\ctxt, #CPU_XREG_OFFSET(8)]
	stp	x10, x11, [\ctxt, #CPU_XREG_OFFSET(10)]
	stp	x12, x13, [\ctxt, #CPU_XREG_OFFSET(12)]
	stp	x14, x15, [\ctxt, #CPU_XREG_OFFSET(14)]
	stp	x16, x17, [\ctxt, #CPU_XREG_OFFSET(16)]
	str	x18,      [\ctxt, #CPU_XREG_OFFSET(18)]
	stp	x19, x20, [\ctxt, #CPU_XREG_OFFSET(19)]
	stp	x21, x22, [\ctxt, #CPU_XREG_OFFSET(21)]
	stp	x23, x24, [\ctxt, #CPU_XREG_OFFSET(23)]
	stp	x25, x26, [\ctxt, #CPU_XREG_OFFSET(25)]
	stp	x27, x28, [\ctxt, #CPU_XREG_OFFSET(27)]
	stp	x29, lr,  [\ctxt, #CPU_XREG_OFFSET(29)]
.endm

.macro restore_all_gp_regs ctxt
	ldp	x2, x3,   [\ctxt, #CPU_XREG_OFFSET(2)]
	ldp	x4, x5,   [\ctxt, #CPU_XREG_OFFSET(4)]
	ldp	x6, x7,   [\ctxt, #CPU_XREG_OFFSET(6)]
	ldp	x8, x9,   [\ctxt, #CPU_XREG_OFFSET(8)]
	ldp	x10, x11, [\ctxt, #CPU_XREG_OFFSET(10)]
	ldp	x12, x13, [\ctxt, #CPU_XREG_OFFSET(12)]
	ldp	x14, x15, [\ctxt, #CPU_XREG_OFFSET(14)]
	ldp	x16, x17, [\ctxt, #CPU_XREG_OFFSET(16)]
	ldr	x18,      [\ctxt, #CPU_XREG_OFFSET(18)]
	ldp	x19, x20, [\ctxt, #CPU_XREG_OFFSET(19)]
	ldp	x21, x22, [\ctxt, #CPU_XREG_OFFSET(21)]
	ldp	x23, x24, [\ctxt, #CPU_XREG_OFFSET(23)]
	ldp	x25, x26, [\ctxt, #CPU_XREG_OFFSET(25)]
	ldp	x27, x28, [\ctxt, #CPU_XREG_OFFSET(27)]
	ldp	x29, lr,  [\ctxt, #CPU_XREG_OFFSET(29)]
.endm

#endif