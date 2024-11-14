/*
 *Description: 
 *Date: 2024-11-11 18:05:01
 */
/*
 *Description: trusted-firmware-A runtime_exception
 *Date: 2024-11-11 18:05:01
 */

#include <s-visor/el3/asm_macros.S>
#include <s-visor/el3/el3_common_macros.S>
#include <s-visor/arch/arm64/arch.h>
#include <s-visor/common/asm.h>
#include <s-visor/lib/el3_runtime/smccc.h>
#include <s-visor/lib/el3_runtime/context.h>
#include <s-visor/el3/runtime_svc.h>

	.globl	runtime_exceptions

	.globl	sync_exception_sp_el0
	.globl	irq_sp_el0
	.globl	fiq_sp_el0
	.globl	serror_sp_el0

	.globl	sync_exception_sp_elx
	.globl	irq_sp_elx
	.globl	fiq_sp_elx
	.globl	serror_sp_elx

	.globl	sync_exception_aarch64
	.globl	irq_aarch64
	.globl	fiq_aarch64
	.globl	serror_aarch64

	.globl	sync_exception_aarch32
	.globl	irq_aarch32
	.globl	fiq_aarch32
	.globl	serror_aarch32

	/*
	 * Macro that prepares entry to EL3 upon taking an exception.
	 *
	 * With RAS_EXTENSION, this macro synchronizes pending errors with an ESB
	 * instruction. When an error is thus synchronized, the handling is
	 * delegated to platform EA handler.
	 *
	 * Without RAS_EXTENSION, this macro synchronizes pending errors using
         * a DSB, unmasks Asynchronous External Aborts and saves X30 before
	 * setting the flag CTX_IS_IN_EL3.
	 *
	 * NOTE: We do not rely on RAS extension in Twinvisor
	 */
	.macro check_and_unmask_ea
	/*
	 * For SoCs which do not implement RAS, use DSB as a barrier to
	 * synchronize pending external aborts.
	 */
	dsb	sy

	/* Unmask the SError interrupt */
	msr	daifclr, #DAIF_ABT_BIT

	/* Use ISB for the above unmask operation to take effect immediately */
	isb

	/*
	 * Refer Note 1. No need to restore X30 as both handle_sync_exception
	 * and handle_interrupt_exception macro which follow this macro modify
	 * X30 anyway.
	 */
	/* TODO: registers */
	str	x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
	mov 	x30, #1
	str	x30, [sp, #CTX_EL3STATE_OFFSET + CTX_IS_IN_EL3]
	dmb	sy

	.endm

	/* ---------------------------------------------------------------------
	 * This macro handles Synchronous exceptions.
	 * Only SMC exceptions are supported.
	 * ---------------------------------------------------------------------
	 */
	.macro	handle_sync_exception

	mrs	x30, esr_el2
	ubfx	x30, x30, #ESR_EC_SHIFT, #ESR_EC_LENGTH

	/* Handle SMC exceptions separately from other synchronous exceptions */
	cmp	x30, #EC_AARCH64_HVC
	b.eq	smc_handler64

	/* Synchronous exceptions other than the above are assumed to be EA */
	ldr	x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]

	b .
	.endm

.globl runtime_exceptions

el3_vector_base runtime_exceptions

	/* ---------------------------------------------------------------------
	 * Current EL with SP_EL0 : 0x0 - 0x200
	 * ---------------------------------------------------------------------
	 */
el3_vector_entry sync_exception_sp_el0
	/* We don't expect any synchronous exceptions from EL3 */
	b	report_unhandled_exception
el3_end_vector_entry sync_exception_sp_el0

el3_vector_entry irq_sp_el0
	/*
	 * EL3 code is non-reentrant. Any asynchronous exception is a serious
	 * error. Loop infinitely.
	 */
	b	report_unhandled_interrupt
el3_end_vector_entry irq_sp_el0

el3_vector_entry fiq_sp_el0
	b	report_unhandled_interrupt
el3_end_vector_entry fiq_sp_el0

el3_vector_entry serror_sp_el0
	// no_ret	plat_handle_el3_ea
	b	report_unhandled_interrupt
el3_end_vector_entry serror_sp_el0

	/* ---------------------------------------------------------------------
	 * Current EL with SP_ELx: 0x200 - 0x400
	 * ---------------------------------------------------------------------
	 */
el3_vector_entry sync_exception_sp_elx
	/* NOTE: n-visor call hvc */
	el3_switch_sp
	apply_at_speculative_wa
	check_and_unmask_ea
	handle_sync_exception
el3_end_vector_entry sync_exception_sp_elx

el3_vector_entry irq_sp_elx
	b	report_unhandled_interrupt
el3_end_vector_entry irq_sp_elx

el3_vector_entry fiq_sp_elx
	b	report_unhandled_interrupt
el3_end_vector_entry fiq_sp_elx

el3_vector_entry serror_sp_elx
	b	report_unhandled_interrupt
el3_end_vector_entry serror_sp_elx

	/* ---------------------------------------------------------------------
	 * Lower EL using AArch64 : 0x400 - 0x600
	 * ---------------------------------------------------------------------
	 */
el3_vector_entry sync_exception_aarch64
	/*
	 * NOTE: s-visor call hvc
	 */
	el3_enable_sp
	apply_at_speculative_wa
	check_and_unmask_ea
	handle_sync_exception
el3_end_vector_entry sync_exception_aarch64

el3_vector_entry irq_aarch64
	b	report_unhandled_interrupt
el3_end_vector_entry irq_aarch64

el3_vector_entry fiq_aarch64
	b	report_unhandled_interrupt
el3_end_vector_entry fiq_aarch64

el3_vector_entry serror_aarch64
	apply_at_speculative_wa
	b	report_unhandled_interrupt
el3_end_vector_entry serror_aarch64

	/* ---------------------------------------------------------------------
	 * Lower EL using AArch32 : 0x600 - 0x800
	 * ---------------------------------------------------------------------
	 */
el3_vector_entry sync_exception_aarch32
	/*
	 * This exception vector will be the entry point for SMCs and traps
	 * that are unhandled at lower ELs most commonly. SP_EL3 should point
	 * to a valid cpu context where the general purpose and system register
	 * state can be saved.
	 */
	b	report_unhandled_interrupt
el3_end_vector_entry sync_exception_aarch32

el3_vector_entry irq_aarch32
	b	report_unhandled_interrupt
el3_end_vector_entry irq_aarch32

el3_vector_entry fiq_aarch32
	b	report_unhandled_interrupt
el3_end_vector_entry fiq_aarch32

el3_vector_entry serror_aarch32
	b	report_unhandled_interrupt
el3_end_vector_entry serror_aarch32


	/* ---------------------------------------------------------------------
	 * The following code handles secure monitor calls.
	 * Depending upon the execution state from where the SMC has been
	 * invoked, it frees some general purpose registers to perform the
	 * remaining tasks. They involve finding the runtime service handler
	 * that is the target of the SMC & switching to runtime stacks (SP_EL0)
	 * before calling the handler.
	 *
	 * Note that x30 has been explicitly saved and can be used here
	 * ---------------------------------------------------------------------
	 */
	 .pushsection .el3.text, "ax"
BEGIN_FUNC(smc_handler)
smc_handler64:
	/* NOTE: The code below must preserve x0-x4 */

	/*
	 * Save general purpose and ARMv8.3-PAuth registers (if enabled).
	 * If Secure Cycle Counter is not disabled in MDCR_EL3 when
	 * ARMv8.5-PMU is implemented, save PMCR_EL0 and disable Cycle Counter.
	 */
	bl	save_gp_pmcr_pauth_regs

	/*
	 * Populate the parameters for the SMC handler.
	 * We already have x0-x4 in place. x5 will point to a cookie (not used
	 * now). x6 will point to the context structure (SP_EL3) and x7 will
	 * contain flags we need to pass to the handler.
	 */
	mov	x5, xzr
	mov	x6, sp

	/*
	 * Restore the saved C runtime stack value which will become the new
	 * SP_EL0 i.e. EL3 runtime stack. It was saved in the 'cpu_context'
	 * structure prior to the last ERET from EL3.
	 */
	ldr	x12, [x6, #CTX_EL3STATE_OFFSET + CTX_RUNTIME_SP]

	/* Switch to SP_EL0 */
	msr	spsel, #MODE_SP_EL0

	/*
	 * Save the SPSR_EL3, ELR_EL3, & SCR_EL3 in case there is a world
	 * switch during SMC handling.
	 * TODO: Revisit if all system registers can be saved later.
	 */
	mrs	x16, spsr_el2
	mrs	x17, elr_el2
	stp	x16, x17, [x6, #CTX_EL3STATE_OFFSET + CTX_SPSR_EL3]
	str	x18, [x6, #CTX_EL3STATE_OFFSET + CTX_SCR_EL3]

	/* Copy SCR_EL3.NS bit to the flag to indicate caller's security
	 * Note:
	 *	In Twinvisor emulation:
	 * 		- Caller from NS world => hvc from el2 => spsr_el2.M[3:0] = 0b1001
	 * 		- Caller from Secure world => hvc from el1 => spsr_el2.M[3:0] = 0b0101
	 *	Hence, we can consider spsr_el2.M[3] as SCR_EL3.NS bit
	 */
	mrs x18, spsr_el2
	lsr x18, x18, #3
	bfi x7, x18, #0, #1

	mov	sp, x12

	/* Get the unique owning entity number */
	ubfx	x16, x0, #FUNCID_OEN_SHIFT, #FUNCID_OEN_WIDTH
	ubfx	x15, x0, #FUNCID_TYPE_SHIFT, #FUNCID_TYPE_WIDTH
	orr	x16, x16, x15, lsl #FUNCID_OEN_WIDTH

	mrs     x9, esr_el2
	ubfx    x9, x9, #0, #15
	cmp     x9, #0 // x9 == #imm, non-zero value means kvm trap
	b.eq    load_svc_desc
	mov     x16, #0x7e 	// titanium_fast idx [0x72,0x7f]
						// set x16 to 0x7e in order to call titanium_smc_handler
load_svc_desc:

	/* Load descriptor index from array of indices */
	adrp	x14, rt_svc_descs_indices
	add	x14, x14, :lo12:rt_svc_descs_indices
	ldrb	w15, [x14, x16] // w15 = 0x1

	/* Any index greater than 127 is invalid. Check bit 7. */
	tbnz	w15, 7, smc_unknown

	/*
	 * Get the descriptor using the index
	 * x11 = (base + off), w15 = index
	 *
	 * handler = (base + off) + (index << log2(size))
	 */
	adrp    x11, __RT_SVC_DESCS_START__
	add x11, x11, :lo12:__RT_SVC_DESCS_START__
	add     x11, x11, RT_SVC_DESC_HANDLE
	// adr	x11, (__RT_SVC_DESCS_START__ + RT_SVC_DESC_HANDLE)
	lsl	w10, w15, #RT_SVC_SIZE_LOG2
	ldr	x15, [x11, w10, uxtw]

	/*
	 * Call the Secure Monitor Call handler and then drop directly into
	 * el3_exit() which will program any remaining architectural state
	 * prior to issuing the ERET to the desired lower EL.
	 */
	blr	x15

	b	el3_exit

smc_unknown:
	/*
	 * Unknown SMC call. Populate return value with SMC_UNK and call
	 * el3_exit() which will restore the remaining architectural state
	 * i.e., SYS, GP and PAuth registers(if any) prior to issuing the ERET
         * to the desired lower EL.
	 */
	mov	x0, #SMC_UNK
	str	x0, [x6, #CTX_GPREGS_OFFSET + CTX_GPREG_X0]
	b	el3_exit

smc_prohibited:
	restore_ptw_el1_sys_regs
	ldp	x28, x29, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X28]
	ldr	x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
	mov	x0, #SMC_UNK
	exception_return

#if DEBUG
rt_svc_fw_critical_error:
	/* Switch to SP_ELx */
	msr	spsel, #MODE_SP_ELX
	no_ret	report_unhandled_exception
#endif
END_FUNC(smc_handler)

BEGIN_FUNC(report_unhandled_exception)
	b .
END_FUNC(report_unhandled_exception)

BEGIN_FUNC(report_unhandled_interrupt)
	b .
END_FUNC(report_unhandled_interrupt)
	.popsection
