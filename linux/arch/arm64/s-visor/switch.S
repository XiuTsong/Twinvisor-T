#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/kvm_arm.h>

#include <s-visor/virt/registers-asm.h>

.macro save_nvisor_gp_regs reg
	add     \reg, \reg, #NVISOR_GP_REGS_OFFSET
	save_all_gp_regs \reg
.endm

.macro restore_nvisor_gp_regs reg
	add     \reg, \reg, #NVISOR_GP_REGS_OFFSET
	restore_all_gp_regs \reg
.endm

.globl global_nvisor_states
.globl __svisor_early_stack
.globl __switchentry

/* x0 = &global_nvisor_states[core_id]
 */
ENTRY(__switch_to_svisor)
    stp         x29, x30, [sp, #-16]!
	str			x1, [sp, #-8]!

	/* switch sp */
	mov x1, sp
	str x1, [x0, #NVISOR_SP_OFFSET]
	ldr x1, [x0, #SVISOR_SP_OFFSET]
	mov sp, x1

	/* save gp regs (except x0 and x1) */
	mov x1, x0
	save_nvisor_gp_regs x0
	mov	x0, x1

	mrs x1, hcr_el2
	str x1, [x0, #NVISOR_HCR_OFFSET]
	/* Disable HCR_EL2.TGE */
	bic	x1, x1, #HCR_TGE
	msr hcr_el2, x1

	/* switch vbar_el2 */
	mrs x1, vbar_el2
	str x1, [x0, #NVISOR_VBAR_OFFSET]
	ldr	x1, =__switchentry
	msr vbar_el2, x1

	/* smc -> eret to s-visor */
	eret
ENDPROC(__switch_to_svisor)

ENTRY(__switch_to_nvisor)
	/* get core id */
	bl __get_core_pos

	/* get &global_nvisor_states[core_id] */
    ldr x1, =global_nvisor_states
	mov w2, #NVISOR_STATE_SIZE
    umaddl x0, w0, w2, x1  // x3 = x1 + w0 * w2

	/* restore vbar_el2 & hcr_el2 */
	ldr x1, [x0, #NVISOR_VBAR_OFFSET]
	msr	vbar_el2, x1
	ldr x1, [x0, #NVISOR_HCR_OFFSET]
	msr hcr_el2, x1

	/* restore gp regs */
	mov x1, x0
	restore_nvisor_gp_regs x0
	mov x0, x1

	/* switch sp */
	mov x1, sp
	str x1, [x0, #SVISOR_SP_OFFSET]
	ldr x1, [x0, #NVISOR_SP_OFFSET]
	mov sp, x1

	ldr	x1, [sp], #8
    ldp x29, x30, [sp], #16

	ret
ENDPROC(__switch_to_nvisor)

ENTRY(__hyp_invalid)
	b	.
ENDPROC(__hyp_invalid)

.macro invalid_vector	label, target = __hyp_invalid
	.align	2
\label:
	b \target
ENDPROC(\label)
.endm

	/* None of these should ever happen */
	invalid_vector	el2t_sync_invalid
	invalid_vector	el2t_irq_invalid
	invalid_vector	el2t_fiq_invalid
	invalid_vector	el2t_error_invalid
	invalid_vector	el2h_sync_invalid
	invalid_vector	el2h_irq_invalid
	invalid_vector	el2h_fiq_invalid
	invalid_vector	el2h_error_invalid
	invalid_vector	el1_sync_invalid
	invalid_vector	el1_irq_invalid
	invalid_vector	el1_fiq_invalid
	invalid_vector	el1_error_invalid

/* Now we come back to n-visor */
el1_sync:
	b __switch_to_nvisor

	/* Should never reach here */
	b .

.macro valid_vect target
	.align 7
	b	\target
.endm

.macro invalid_vect target
	.align 7
	b	\target
.endm

	.pushsection ".switchentry.text", "ax"

ENTRY(switch_vectors)
	invalid_vect	el2t_sync_invalid	// Synchronous EL2t
	invalid_vect	el2t_irq_invalid	// IRQ EL2t
	invalid_vect	el2t_fiq_invalid	// FIQ EL2t
	invalid_vect	el2t_error_invalid	// Error EL2t

	invalid_vect	el2h_sync_invalid		// Synchronous EL2h
	invalid_vect	el2h_irq_invalid	// IRQ EL2h
	invalid_vect	el2h_fiq_invalid	// FIQ EL2h
	invalid_vect	el2h_error_invalid		// Error EL2h

	valid_vect	el1_sync		// Synchronous 64-bit EL1
	invalid_vect	el1_irq_invalid			// IRQ 64-bit EL1
	invalid_vect	el1_fiq_invalid		// FIQ 64-bit EL1
	invalid_vect	el1_error_invalid		// Error 64-bit EL1

	invalid_vect	el1_sync_invalid		// Synchronous 32-bit EL1
	invalid_vect	el1_irq_invalid			// IRQ 32-bit EL1
	invalid_vect	el1_fiq_invalid		// FIQ 32-bit EL1
	invalid_vect	el1_error_invalid		// Error 32-bit EL1
END(switch_vectors)

	.popsection
