/*
 *Description: titanium guest-entry.S
 *Date: 2024-11-08 15:19:56
 */
#include <asm/asm-offsets.h>

#include <s-visor/common/asm.h>
#include <s-visor/virt/registers-asm.h>
#include <s-visor/virt/vmexit_def.h>
#include <s-visor/virt/sec_defs.h>
#include <s-visor/arch/arm64/arch.h>
#include <s-visor/arch/arm64/misc.h>
#include <s-visor/arch/arm64/assembler.h>
#include <s-visor/mm/sec_shm.h>

// .extern _boot_pt_l0_0
.extern exception_handler

/* Use ttbr0_el1 to protect x0 since
 * s-visor only use ttbr0_el1 for uart
 */
.macro naive_save_reg reg
	msr ttbr0_el1, \reg
.endm

.macro naive_restore_reg reg
	mrs \reg, ttbr0_el1
.endm

/* Save and restore @reg using dbgbcr<n>_el1.
 * Make sure @tmp_reg has been protected before using this macro.
 */
.macro naive_save_reg_0 reg, tmp_reg
	ubfx    \tmp_reg, \reg, #0, #32
	lsl     \tmp_reg, \tmp_reg, #32
	msr     dbgbcr0_el1, \tmp_reg
	ubfx    \tmp_reg, \reg, #32, #32
	lsl     \tmp_reg, \tmp_reg, #32
	msr     dbgbcr1_el1, \tmp_reg
.endm

.macro naive_save_reg_1 reg, tmp_reg
	ubfx    \tmp_reg, \reg, #0, #32
	lsl     \tmp_reg, \tmp_reg, #32
	msr     dbgbcr2_el1, \tmp_reg
	ubfx    \tmp_reg, \reg, #32, #32
	lsl     \tmp_reg, \tmp_reg, #32
	msr     dbgbcr3_el1, \tmp_reg
.endm

.macro naive_restore_reg_0 reg, tmp_reg
	mrs     \reg, dbgbcr1_el1
	mrs     \tmp_reg, dbgbcr0_el1
	lsr     \tmp_reg, \tmp_reg, #32
	orr     \reg, \reg, \tmp_reg
.endm

.macro naive_restore_reg_1 reg, tmp_reg
	mrs     \reg, dbgbcr3_el1
	mrs     \tmp_reg, dbgbcr2_el1
	lsr     \tmp_reg, \tmp_reg, #32
	orr     \reg, \reg, \tmp_reg
.endm

.macro naive_save_x0_x1_x2
	naive_save_reg x0
	naive_save_reg_0 x1, x0
	naive_save_reg_1 x2, x0
.endm

.macro naive_save_x1_x2
	naive_save_reg_0 x1, x0
	naive_save_reg_1 x2, x0
.endm

.macro naive_restore_x0_x1_x2
	naive_restore_reg_1 x2, x0
	naive_restore_reg_0 x1, x0
	naive_restore_reg x0
.endm

/* Called after x0, x1, x2 are protected */
.macro switch_tpidr_el1
	__get_core_id x0, x1, x2
	ldr x1, =g_tpidr
	mov w2, #8
	umaddl x1, w0, w2, x1 // x1 = g_tpidr[core_id]
	ldr x0, [x1] // x0 = s-visor's tpidr_el1
	mrs x2, tpidr_el1 // x2 = guest's tpidr_el1
	msr tpidr_el1, x0
	str x2, [x1] // save guest's tpidr_el1 to g_tpidr[core_id]
.endm

/* switch ttbr1? */
.macro switch_ttbr1_el1
	get_shm_base x0
	ldr x0, [x0, #SHM_TTBR_PHYS_OFFSET]
	/* switch page table */
	msr ttbr1_el1, x0
	tlbi vmalle1
	dsb sy
	isb
.endm

.macro get_current_ttbr0 reg
	mrs    \reg, tpidr_el1
	add    \reg, \reg, #ENTRY_HELPER_OFFSET
	ldr    \reg, [\reg, #GUEST_TTBR0_OFFSET]
.endm

.macro get_current_ttbr1 reg
	mrs    \reg, tpidr_el1
	add    \reg, \reg, #ENTRY_HELPER_OFFSET
	ldr    \reg, [\reg, #GUEST_TTBR1_OFFSET]
.endm

.macro get_jump_vbar_flag reg
	mrs    \reg, tpidr_el1
	add    \reg, \reg, #ENTRY_HELPER_OFFSET
	ldr    \reg, [\reg, #JUMP_VBAR_OFFSET]
.endm

/* Check elr_el1 range
 * s-visor address range: 0xe1xxxxx
 * FIXME: Hard code here
 */
.macro check_elr_range reg0, reg1
	mrs    \reg0, elr_el1
	lsr    \reg0, \reg0, #20
	mov    \reg1, #0xe1
	cmp    \reg0, \reg1
.endm

	.pushsection ".svisor.text", "ax"
el1h_sync:
	naive_save_x1_x2
	switch_tpidr_el1
	naive_restore_x0_x1_x2
	save_guest_states_without_stack
	mov x0, #TITANIUM_VMEXIT_SYNC
	b exit_guest

el1h_irq:
	naive_save_x1_x2
	switch_tpidr_el1
	naive_restore_x0_x1_x2
	save_guest_states_without_stack
	mov x0, #TITANIUM_VMEXIT_IRQ
	b exit_guest

el1h_fiq:
	naive_save_x1_x2
	switch_tpidr_el1
	naive_restore_x0_x1_x2
	save_guest_states_without_stack
	mov x0, #TITANIUM_VMEXIT_FIQ
	b exit_guest

el1h_error:
	naive_save_x1_x2
	switch_tpidr_el1
	naive_restore_x0_x1_x2
	mov x0, #TITANIUM_VMEXIT_ERR
	b exit_guest

el0_sync:
	naive_save_x1_x2
	switch_tpidr_el1
	naive_restore_x0_x1_x2
	save_guest_states_without_stack
	mov x0, #TITANIUM_VMEXIT_USER_SYNC
	b exit_guest

el0_irq:
	naive_save_x1_x2
	switch_tpidr_el1
	naive_restore_x0_x1_x2
	save_guest_states_without_stack
	mov x0, #TITANIUM_VMEXIT_USER_IRQ
	b exit_guest

el0_fiq:
	naive_save_x1_x2
	switch_tpidr_el1
	naive_restore_x0_x1_x2
	save_guest_states_without_stack
	mov x0, #TITANIUM_VMEXIT_USER_FIQ
	b exit_guest

.macro invalid_vector	label, target = hyp_panic
	.align	2
\label:
	b \target
ENDPROC(\label)
.endm

	/* None of these should ever happen */
	invalid_vector	el1t_sync_invalid
	invalid_vector	el1t_irq_invalid
	invalid_vector	el1t_fiq_invalid
	invalid_vector	el1t_error_invalid
	invalid_vector	el0_sync_invalid
	invalid_vector	el0_irq_invalid
	invalid_vector	el0_fiq_invalid
	invalid_vector	el0_error_invalid

	.ltorg

	.align 11

.macro get_shm_base reg
	_mov_q \reg, GUEST_SHM_ADDR_BASE
.endm

.macro get_shared_mem_base reg0, offset_reg
	get_shm_base \reg0
	add \reg0, \reg0, \offset_reg
.endm

.macro get_val_from_shared_mem reg, base_reg, offset
	ldr \reg, [\base_reg, \offset]
.endm

.macro valid_vect target
	.align 7
	naive_save_reg x0
	switch_ttbr1_el1
	ldr x0, =\target
	br x0
.endm

.macro jump_vect offset
	.align 7
	stp x0, x1, [sp, #-16]!
	str x2, [sp, #-8]!
	__get_core_id x1, x0, x2
	get_shm_base x0
	mov x2, #0x1000
	umaddl x0, w1, w2, x0  // x3 = x1 + w0 * w2
	ldr x0, [x0, #SHM_GUEST_VECTOR_OFFSET]
	add x0, x0, \offset
	br x0
.endm

.macro invalid_vect target
	.align 7
	b	\target
.endm

ENTRY_4K(titanium_hyp_vector)
	invalid_vect	el1t_sync_invalid	// Synchronous EL1t
	invalid_vect	el1t_irq_invalid	// IRQ EL1t
	invalid_vect	el1t_fiq_invalid	// FIQ EL1t
	invalid_vect	el1t_error_invalid	// Error EL1t

	valid_vect		el1h_sync			// Synchronous EL1h
	valid_vect		el1h_irq			// IRQ EL1h
	valid_vect		el1h_fiq			// FIQ EL1h
	valid_vect		el1h_error			// Error EL1h

	jump_vect		#0x400				// Synchronous 64-bit EL0
	valid_vect		el0_irq				// IRQ 64-bit EL0
	valid_vect		el0_fiq				// FIQ 64-bit EL0
	invalid_vect	el0_error_invalid	// Error 64-bit EL0

	invalid_vect	el0_sync_invalid	// Synchronous 32-bit EL0
	invalid_vect	el0_irq_invalid		// IRQ 32-bit EL0
	invalid_vect	el0_fiq_invalid		// FIQ 32-bit EL0
	invalid_vect	el0_error_invalid	// Error 32-bit EL0
ENDPROC(titanium_hyp_vector)

BEGIN_GATE(switch_gate)
to_vm_eret:
	get_current_ttbr1 x0
	msr ttbr1_el1, x0
	tlbi vmalle1
	dsb sy
	isb

	get_shared_mem_base x0, x1 // Now x0 point to shared mem base address
	/* Restore tpidr_el1, x1, x0. */
	ldr x1, [x0, #SHM_TPIDR_OFFSET]
	msr tpidr_el1, x1
	ldp x0, x1, [x0, #SHM_X0_OFFSET]

	eret

.align JUMP_OFFSET_ORDER
to_vm_jump:
	get_current_ttbr1 x0
	msr ttbr1_el1, x0
	tlbi vmalle1
	dsb sy
	isb

	get_shared_mem_base x0, x1 // x0 point to shared mem base address
	/* Only restore tpidr_el1 here
	 * x1, x0 will be restored in guest's kernel_ventry_1
	 */
	ldr x1, [x0, #SHM_TPIDR_OFFSET]
	msr tpidr_el1, x1

	/* x1 point to g_vbar_target */
	ldr x1, [x0, #SHM_VBAR_TARGET_OFFSET]

	/* Jump to guest vbar entry */
	br x1
END_GATE(switch_gate)

/* x0 == core_id */
BEGIN_FUNC(enter_guest)
	/* We call nested functions, follow the ABI. */
	stp x29, x30, [sp, #-16]!

	/*
	 * We do not save s-visor's gp regs here since
	 * we don't need to restore them in exit_guest().
	 * Because according to the assembly code of forward_smc_to_vm()，
	 * all the registers are protected in stack.
	 * We only need to save sp here.
	 */
	get_host_sys_regs x0
	mov x1, sp
	str x1, [x0, #SYS_SP_OFFSET]

	mov x0, #SHARED_MEM_SIZE
	mul x1, x0, x2 // x1 == per-core shared mem offset
	naive_save_reg x1

	/*
	 * Restore guest's state except for x0, x1, tpidr_el1
	 * which are saved in the shared memory in __prepared_shared_mem().
	 */
	naive_restore_guest_states x0, x1
	naive_restore_reg x1

	get_current_ttbr0 x0
	msr ttbr0_el1, x0

	get_jump_vbar_flag x0
	cmp x0, #0
	_mov_q x0, SWITCH_GATE_HIGH
	b.eq normal_switch

	_mov_q x0, SWITCH_GATE_JUMP_HIGH

normal_switch:
	br x0
END_FUNC(enter_guest)

BEGIN_FUNC(exit_guest)
	/* restore s-visor's stack */
	get_host_sys_regs x1
	ldr x2, [x1, #SYS_SP_OFFSET]
	mov sp, x2

	ldp x29, x30, [sp], #16
	ret
END_FUNC(exit_guest)
	.popsection
