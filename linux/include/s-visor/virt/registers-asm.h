/*
 * Created on 2024/11/05
 */

#ifndef __SVISOR_REGISTERS_ASM_H__
#define __SVISOR_REGISTERS_ASM_H__

#include <asm/asm-offsets.h>
#include <s-visor/common/asm.h>

.macro get_guest_gp_regs reg
        mrs     \reg, tpidr_el1
        add     \reg, \reg, #VCPU_CTX_OFFSET
        add     \reg, \reg, #GUEST_GP_REGS_OFFSET
.endm

.macro get_guest_sys_regs reg
        mrs     \reg, tpidr_el1
        add     \reg, \reg, #VCPU_CTX_OFFSET
        add     \reg, \reg, #GUEST_SYS_REGS_OFFSET
.endm

.macro get_host_gp_regs reg
        mrs     \reg, tpidr_el1
        add     \reg, \reg, #HOST_STATE_OFFSET
        add     \reg, \reg, #HOST_GP_REGS_OFFSET
.endm

.macro get_host_sys_regs reg
        mrs     \reg, tpidr_el1
        add     \reg, \reg, #HOST_STATE_OFFSET
        add     \reg, \reg, #HOST_SYS_REGS_OFFSET
.endm

.macro save_callee_saved_gp_regs ctxt
	stp	x19, x20, [\ctxt, #CPU_XREG_OFFSET(19)]
	stp	x21, x22, [\ctxt, #CPU_XREG_OFFSET(21)]
	stp	x23, x24, [\ctxt, #CPU_XREG_OFFSET(23)]
	stp	x25, x26, [\ctxt, #CPU_XREG_OFFSET(25)]
	stp	x27, x28, [\ctxt, #CPU_XREG_OFFSET(27)]
	stp	x29, lr,  [\ctxt, #CPU_XREG_OFFSET(29)]
.endm

.macro restore_callee_saved_gp_regs ctxt
	ldp	x19, x20, [\ctxt, #CPU_XREG_OFFSET(19)]
	ldp	x21, x22, [\ctxt, #CPU_XREG_OFFSET(21)]
	ldp	x23, x24, [\ctxt, #CPU_XREG_OFFSET(23)]
	ldp	x25, x26, [\ctxt, #CPU_XREG_OFFSET(25)]
	ldp	x27, x28, [\ctxt, #CPU_XREG_OFFSET(27)]
	ldp	x29, lr,  [\ctxt, #CPU_XREG_OFFSET(29)]
.endm

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

.macro save_guest_states tmp0, tmp1, tmp2
    stp         \tmp0, \tmp1, [sp, #-16]! // save x0 and x1 to stack
    get_guest_gp_regs \tmp0
    save_all_gp_regs \tmp0
    ldp         \tmp1, \tmp2, [sp], #16 // restore x0, x1
    stp	        \tmp1, \tmp2, [\tmp0, #CPU_XREG_OFFSET(0)]

    get_guest_sys_regs \tmp0
    mrs         \tmp1, spsr_el1 
    str	        \tmp1, [\tmp0, #SYS_SPSR_OFFSET]
    mrs         \tmp1, elr_el1
    str	        \tmp1, [\tmp0, #SYS_ELR_OFFSET]
    mrs         \tmp1, sp_el0
    str	        \tmp1, [\tmp0, #SYS_SP_EL0_OFFSET]
    mrs         \tmp1, esr_el1
    str	        \tmp1, [\tmp0, #SYS_ESR_OFFSET]
    mrs         \tmp1, tpidr_el1
    str	        \tmp1, [\tmp0, #SYS_TPIDR_OFFSET]
    mrs         \tmp1, sctlr_el1
    str	        \tmp1, [\tmp0, #SYS_SCTLR_OFFSET]
    mrs         \tmp1, mair_el1
    str	        \tmp1, [\tmp0, #SYS_MAIR_OFFSET]
    mrs         \tmp1, amair_el1
    str	        \tmp1, [\tmp0, #SYS_AMAIR_OFFSET]
    mrs         \tmp1, tcr_el1
    str	        \tmp1, [\tmp0, #SYS_TCR_OFFSET]

.endm

.macro restore_guest_states tmp0, tmp1
    get_guest_sys_regs \tmp0
    ldr	        \tmp1, [\tmp0, #SYS_SPSR_OFFSET]
    msr         spsr_el1, \tmp1 
    ldr	        \tmp1, [\tmp0, #SYS_ELR_OFFSET]
    msr         elr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_SP_EL0_OFFSET]
    msr         sp_el0, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_ESR_OFFSET]
    msr         esr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_TPIDR_OFFSET]
    msr         tpidr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_SCTLR_OFFSET]
    msr         sctlr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_MAIR_OFFSET]
    msr         mair_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_AMAIR_OFFSET]
    msr         amair_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_TCR_OFFSET]
    msr         tcr_el1, \tmp1
    get_guest_gp_regs \tmp0
    restore_all_gp_regs \tmp0
    ldp         \tmp0, \tmp1, [\tmp0, #CPU_XREG_OFFSET(0)]
.endm

.macro restore_guest_states_except_esr tmp0, tmp1
    get_guest_sys_regs \tmp0
    ldr	        \tmp1, [\tmp0, #SYS_SPSR_OFFSET]
    msr         spsr_el1, \tmp1 
    ldr	        \tmp1, [\tmp0, #SYS_ELR_OFFSET]
    msr         elr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_SP_EL0_OFFSET]
    msr         sp_el0, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_TPIDR_OFFSET]
    msr         tpidr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_SCTLR_OFFSET]
    msr         sctlr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_MAIR_OFFSET]
    msr         mair_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_AMAIR_OFFSET]
    msr         amair_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_TCR_OFFSET]
    msr         tcr_el1, \tmp1
    get_guest_gp_regs \tmp0
    restore_all_gp_regs \tmp0
    ldp         \tmp0, \tmp1, [\tmp0, #CPU_XREG_OFFSET(0)]
.endm

.macro restore_host_states tmp0, tmp1
    get_host_gp_regs \tmp0
    restore_all_gp_regs \tmp0
    ldp         \tmp0, \tmp1, [\tmp0, #CPU_XREG_OFFSET(0)]
.endm

.macro save_guest_sys_regs tmp0, tmp1
    mrs         \tmp1, spsr_el1
    str	        \tmp1, [\tmp0, #SYS_SPSR_OFFSET]
    mrs         \tmp1, sctlr_el1
    str	        \tmp1, [\tmp0, #SYS_SCTLR_OFFSET]
    mrs         \tmp1, sp_el1
    str	        \tmp1, [\tmp0, #SYS_SP_OFFSET]
    mrs         \tmp1, sp_el0
    str	        \tmp1, [\tmp0, #SYS_SP_EL0_OFFSET]
    mrs         \tmp1, esr_el1
    str	        \tmp1, [\tmp0, #SYS_ESR_OFFSET]
    mrs         \tmp1, vbar_el1
    str	        \tmp1, [\tmp0, #SYS_VBAR_OFFSET]
    mrs         \tmp1, ttbr0_el12
    str	        \tmp1, [\tmp0, #SYS_TTBR0_OFFSET]
    mrs         \tmp1, ttbr1_el12
    str	        \tmp1, [\tmp0, #SYS_TTBR1_OFFSET]
    mrs         \tmp1, mair_el1
    str	        \tmp1, [\tmp0, #SYS_MAIR_OFFSET]
    mrs         \tmp1, amair_el1
    str	        \tmp1, [\tmp0, #SYS_AMAIR_OFFSET]
    mrs         \tmp1, tcr_el1
    str	        \tmp1, [\tmp0, #SYS_TCR_OFFSET]
    mrs         \tmp1, tpidr_el1
    str	        \tmp1, [\tmp0, #SYS_TPIDR_OFFSET]
.endm

.macro restore_guest_sys_regs tmp0, tmp1
    ldr	        \tmp1, [\tmp0, #SYS_SPSR_OFFSET]
    msr         spsr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_SCTLR_OFFSET]
    msr         sctlr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_SP_OFFSET]
    msr         sp_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_SP_EL0_OFFSET]
    msr         sp_el0, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_ESR_OFFSET]
    msr         esr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_VBAR_OFFSET]
    msr         vbar_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_TTBR0_OFFSET]
    msr         ttbr0_el12, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_TTBR1_OFFSET]
    msr         ttbr1_el12, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_MAIR_OFFSET]
    msr         mair_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_AMAIR_OFFSET]
    msr         amair_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_TCR_OFFSET]
    msr         tcr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_TPIDR_OFFSET]
    msr         tpidr_el1, \tmp1
    isb
.endm

.macro save_guest_states_without_stack
    msr         ttbr1_el1, x0
    get_guest_gp_regs x0
    save_all_gp_regs x0
    mrs         x2, ttbr1_el1
    stp	        x2, x1, [x0, #CPU_XREG_OFFSET(0)]

    get_guest_sys_regs x0
    mrs         x1, spsr_el1
    str	        x1, [x0, #SYS_SPSR_OFFSET]
    mrs         x1, elr_el1
    str	        x1, [x0, #SYS_ELR_OFFSET]
    mrs         x1, sp_el0
    str	        x1, [x0, #SYS_SP_EL0_OFFSET]
    mrs         x1, esr_el1
    str	        x1, [x0, #SYS_ESR_OFFSET]
    mrs         x1, tpidr_el1
    str	        x1, [x0, #SYS_TPIDR_OFFSET]
    mrs         x1, sctlr_el1
    str	        x1, [x0, #SYS_SCTLR_OFFSET]
    mrs         x1, mair_el1
    str	        x1, [x0, #SYS_MAIR_OFFSET]
    mrs         x1, amair_el1
    str	        x1, [x0, #SYS_AMAIR_OFFSET]
    mrs         x1, tcr_el1
    str	        x1, [x0, #SYS_TCR_OFFSET]
    mrs         x1, far_el1
    str         x1, [x0, #SYS_FAR_OFFSET]
    /* save stack(sp_el1 is not writable in el1) */
    mov         x1, sp
    str	        x1, [x0, #SYS_SP_OFFSET]
.endm

/* restore sp; but not restore x0, x1, tpidr_el1*/
.macro naive_restore_guest_states tmp0, tmp1
    get_guest_sys_regs \tmp0
    ldr	        \tmp1, [\tmp0, #SYS_SPSR_OFFSET]
    msr         spsr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_ELR_OFFSET]
    msr         elr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_SP_EL0_OFFSET]
    msr         sp_el0, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_ESR_OFFSET]
    msr         esr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_SCTLR_OFFSET]
    msr         sctlr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_MAIR_OFFSET]
    msr         mair_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_AMAIR_OFFSET]
    msr         amair_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_TCR_OFFSET]
    msr         tcr_el1, \tmp1
    ldr	        \tmp1, [\tmp0, #SYS_SP_OFFSET]
    mov         sp, \tmp1
    get_guest_gp_regs \tmp0
    restore_all_gp_regs \tmp0
.endm

#endif