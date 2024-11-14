#include <linux/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/assembler.h>
#include <asm/kvm_arm.h>

#include <s-visor/virt/registers-asm.h>
#include <s-visor/arch/arm64/misc.h>
#include <s-visor/lib/el3_runtime/context.h>
#include <s-visor/el3/asm_macros.S>
#include <s-visor/el3/el3_common_macros.S>

	.pushsection ".el3.text", "ax"

	.global	el1_sysregs_context_save
	.global	el1_sysregs_context_restore
	.global	el2_sysregs_context_save
	.global	el2_sysregs_context_restore
	.global	el2_sysregs_context_save_host_only
	.global	el2_sysregs_context_restore_host_only

/* -----------------------------------------------------
 * The following function strictly follows the AArch64
 * PCS to use x9-x17 (temporary caller-saved registers)
 * to save EL2 system register context. It assumes that
 * 'x0' is pointing to a 'el2_sys_regs' structure where
 * the register context will be saved.
 * -----------------------------------------------------
 */
BEGIN_FUNC(el2_sysregs_context_save_host_only)
	mrs	x9, spsr_el2
	mrs	x10, elr_el2
	stp	x9, x10, [x0, #CTX_SPSR_EL2]

	mrs	x11, esr_el2
	str x11, [x0, #CTX_ESR_EL2]

	ret
END_FUNC(el2_sysregs_context_save_host_only)

/* -----------------------------------------------------
 * The following function strictly follows the AArch64
 * PCS to use x9-x17 (temporary caller-saved registers)
 * to restore EL2 system register context.  It assumes
 * that 'x0' is pointing to a 'el2_sys_regs' structure
 * from where the register context will be restored
 * -----------------------------------------------------
 */
BEGIN_FUNC(el2_sysregs_context_restore_host_only)
	ldp	x9, x10, [x0, #CTX_SPSR_EL2]
	msr	spsr_el2, x9
	msr	elr_el2, x10

	ldr x11, [x0, #CTX_ESR_EL2]
	msr	esr_el2, x11

	ret
END_FUNC(el2_sysregs_context_restore_host_only)

/* -----------------------------------------------------
 * The following function strictly follows the AArch64
 * PCS to use x9-x17 (temporary caller-saved registers)
 * to save EL2 system register context. It assumes that
 * 'x0' is pointing to a 'el2_sys_regs' structure where
 * the register context will be saved.
 * -----------------------------------------------------
 */
BEGIN_FUNC(el2_sysregs_context_save)
	mrs	x9, spsr_el2
	mrs	x10, elr_el2
	stp	x9, x10, [x0, #CTX_SPSR_EL2]

	mrs	x15, sctlr_el2
	mrs	x16, actlr_el2
	stp	x15, x16, [x0, #CTX_SCTLR_EL2]

	mrs	x17, vsttbr_el2
	mrs	x9,  vstcr_el2
	stp	x17, x9, [x0, #CTX_VSTTBR_EL2]

	mrs	x17, vttbr_el2
	mrs	x9,  vtcr_el2
	stp	x17, x9, [x0, #CTX_VTTBR_EL2]

	mrs	x10, sp_el2
	mrs	x11, esr_el2
	stp	x10, x11, [x0, #CTX_SP_EL2]

	mrs	x12, ttbr0_el2
	mov	x13, xzr
/*	mrs	x13, vsttbr_el2  		access vsttbr_el2 cause program crush */
	stp	x12, x13, [x0, #CTX_TTBR0_EL2]

	mrs	x14, mair_el2
	mrs	x15, amair_el2
	stp	x14, x15, [x0, #CTX_MAIR_EL2]

	mrs	x16, tcr_el2
	mrs	x17, tpidr_el0
	stp	x16, x17, [x0, #CTX_TCR_EL2]

	mrs	x9, tpidr_el1
	mrs	x10, tpidr_el2
	stp	x9, x10, [x0, #CTX_TPIDR_SEL1]

	mrs	x13, par_el1
	mrs	x14, hpfar_el2
	stp	x13, x14, [x0, #CTX_PAR_SEL1]

	mrs	x13, far_el2
	mrs	x14, afsr0_el2
	stp	x13, x14, [x0, #CTX_FAR_EL2]

	mrs	x15, afsr1_el2
	mrs	x16, contextidr_el2
	stp	x15, x16, [x0, #CTX_AFSR1_EL2]

	mrs	x17, vbar_el2
	mrs	x9, hcr_el2
	stp	x17, x9, [x0, #CTX_VBAR_EL2]

	mrs	x17, cnthctl_el2
	mrs	x9, cnthp_ctl_el2
	stp	x17, x9, [x0, #CTX_CNTHCTL_EL2]

	mrs	x17, cnthp_cval_el2
	mrs	x9, cnthp_tval_el2
	stp	x17, x9, [x0, #CTX_CNTHP_CVAL_EL2]

	mrs	x17, cnthv_ctl_el2
	mrs	x9, cnthv_cval_el2
	stp	x17, x9, [x0, #CTX_CNTHV_CTL_EL2]

	mrs	x17, cnthv_tval_el2
	str	x17, [x0, #CTX_CNTHV_TVAL_EL2]
	ret
END_FUNC(el2_sysregs_context_save)

/* -----------------------------------------------------
 * The following function strictly follows the AArch64
 * PCS to use x9-x17 (temporary caller-saved registers)
 * to restore EL2 system register context.  It assumes
 * that 'x0' is pointing to a 'el2_sys_regs' structure
 * from where the register context will be restored
 * -----------------------------------------------------
 */
BEGIN_FUNC(el2_sysregs_context_restore)
	ldp	x9, x10, [x0, #CTX_SPSR_EL2]
	msr	spsr_el2, x9
	msr	elr_el2, x10

	ldp	x15, x16, [x0, #CTX_SCTLR_EL2]
	msr	sctlr_el2, x15
	msr	actlr_el2, x16

	ldp	x17, x9, [x0, #CTX_VSTTBR_EL2]
	msr	vsttbr_el2, x17
	msr	vstcr_el2, x9

	ldp	x17, x9, [x0, #CTX_VTTBR_EL2]
	msr	vttbr_el2, x17
	msr	vtcr_el2, x9

	ldp	x10, x11, [x0, #CTX_SP_EL2]
	msr	sp_el2, x10
	msr	esr_el2, x11

	ldp	x12, x13, [x0, #CTX_TTBR0_EL2]
	msr	ttbr0_el2, x12

	ldp	x14, x15, [x0, #CTX_MAIR_EL2]
	msr	mair_el2, x14
	msr	amair_el2, x15

	ldp	x16, x17, [x0, #CTX_TCR_EL2]
	msr	tcr_el2, x16
	msr	tpidr_el0, x17

	ldp	x9, x10, [x0, #CTX_TPIDR_SEL1]
	msr	tpidr_el1, x9
	msr	tpidr_el2, x10

	ldp	x13, x14, [x0, #CTX_PAR_SEL1]
	msr	par_el1, x13
	msr	hpfar_el2, x14

	ldp	x13, x14, [x0, #CTX_FAR_EL2]
	msr	far_el2, x13
	msr	afsr0_el2, x14

	ldp	x15, x16, [x0, #CTX_AFSR1_EL2]
	msr	afsr1_el2, x15
	msr	contextidr_el2, x16

	ldp	x17, x9, [x0, #CTX_VBAR_EL2]
	msr	vbar_el2, x17
	msr	hcr_el2, x9

	ldp	x17, x9, [x0, #CTX_CNTHCTL_EL2]
	msr	cnthctl_el2, x17
	msr	cnthp_ctl_el2, x9

	ldp	x17, x9, [x0, #CTX_CNTHP_CVAL_EL2]
	msr	cnthp_cval_el2, x17
	msr	cnthp_tval_el2, x9

	ldp	x17, x9, [x0, #CTX_CNTHV_CTL_EL2]
	msr	cnthv_ctl_el2, x17
	msr	cnthv_cval_el2, x9

	ldr	x17, [x0, #CTX_CNTHV_TVAL_EL2]
	msr	cnthv_tval_el2, x17
	/* No explict ISB required here as ERET covers it */
	ret
END_FUNC(el2_sysregs_context_restore)

/*
 * Save el1 sysregs in EL2 when HCR_EL2.E2H enabled.
 * Port from trusted-firmware-a
 */
BEGIN_FUNC(el1_sysregs_context_save)
	mrs	x9, spsr_el12
	mrs	x10, elr_el12
	stp	x9, x10, [x0, #CTX_SPSR_EL1]

	mrs	x15, sctlr_el12
	mrs	x16, tcr_el12
	stp	x15, x16, [x0, #CTX_SCTLR_EL1]

	mrs	x17, cpacr_el12
	mrs	x9, csselr_el1
	stp	x17, x9, [x0, #CTX_CPACR_EL1]

	mrs	x10, sp_el1
	mrs	x11, esr_el12
	stp	x10, x11, [x0, #CTX_SP_EL1]

	mrs	x12, ttbr0_el12
	mrs	x13, ttbr1_el12
	stp	x12, x13, [x0, #CTX_TTBR0_EL1]

	mrs	x14, mair_el12
	mrs	x15, amair_el12
	stp	x14, x15, [x0, #CTX_MAIR_EL1]

	mrs	x16, actlr_el1
	mrs	x17, tpidr_el1
	stp	x16, x17, [x0, #CTX_ACTLR_EL1]

	mrs	x9, tpidr_el0
	mrs	x10, tpidrro_el0
	stp	x9, x10, [x0, #CTX_TPIDR_EL0]

	mrs	x13, par_el1
	mrs	x14, far_el12
	stp	x13, x14, [x0, #CTX_PAR_EL1]

	mrs	x15, afsr0_el12
	mrs	x16, afsr1_el12
	stp	x15, x16, [x0, #CTX_AFSR0_EL1]

	mrs	x17, contextidr_el12
	mrs	x9, vbar_el12
	stp	x17, x9, [x0, #CTX_CONTEXTIDR_EL1]

	/* Save NS timer registers if the build has instructed so */
#if NS_TIMER_SWITCH
	mrs	x10, cntp_ctl_el0
	mrs	x11, cntp_cval_el0
	stp	x10, x11, [x0, #CTX_CNTP_CTL_EL0]

	mrs	x12, cntv_ctl_el0
	mrs	x13, cntv_cval_el0
	stp	x12, x13, [x0, #CTX_CNTV_CTL_EL0]

	mrs	x14, cntkctl_el12
	str	x14, [x0, #CTX_CNTKCTL_EL1]
#endif

	ret
END_FUNC(el1_sysregs_context_save)

BEGIN_FUNC(el1_sysregs_context_restore)
	ldp	x9, x10, [x0, #CTX_SPSR_EL1]
	msr	spsr_el12, x9
	msr	elr_el12, x10

	ldp	x15, x16, [x0, #CTX_SCTLR_EL1]
	msr	sctlr_el12, x15
	msr	tcr_el12, x16

	ldp	x17, x9, [x0, #CTX_CPACR_EL1]
	msr	cpacr_el12, x17
	msr	csselr_el1, x9

	ldp	x10, x11, [x0, #CTX_SP_EL1]
	msr	sp_el1, x10
	msr	esr_el12, x11

	ldp	x12, x13, [x0, #CTX_TTBR0_EL1]
	msr	ttbr0_el12, x12
	msr	ttbr1_el12, x13

	ldp	x14, x15, [x0, #CTX_MAIR_EL1]
	msr	mair_el12, x14
	msr	amair_el12, x15

	ldp 	x16, x17, [x0, #CTX_ACTLR_EL1]
	msr	actlr_el1, x16
	msr	tpidr_el1, x17

	ldp	x9, x10, [x0, #CTX_TPIDR_EL0]
	msr	tpidr_el0, x9
	msr	tpidrro_el0, x10

	ldp	x13, x14, [x0, #CTX_PAR_EL1]
	msr	par_el1, x13
	msr	far_el12, x14

	ldp	x15, x16, [x0, #CTX_AFSR0_EL1]
	msr	afsr0_el12, x15
	msr	afsr1_el12, x16

	ldp	x17, x9, [x0, #CTX_CONTEXTIDR_EL1]
	msr	contextidr_el12, x17
	msr	vbar_el12, x9

	/* Restore NS timer registers if the build has instructed so */
#if NS_TIMER_SWITCH
	ldp	x10, x11, [x0, #CTX_CNTP_CTL_EL0]
	msr	cntp_ctl_el0, x10
	msr	cntp_cval_el0, x11

	ldp	x12, x13, [x0, #CTX_CNTV_CTL_EL0]
	msr	cntv_ctl_el0, x12
	msr	cntv_cval_el0, x13

	ldr	x14, [x0, #CTX_CNTKCTL_EL1]
	msr	cntkctl_el12, x14
#endif

	/* No explict ISB required here as ERET covers it */
	ret
END_FUNC(el1_sysregs_context_restore)

/*
 * Save el1 sysregs in EL1
 * Port from trusted-firmware-a
 */
BEGIN_FUNC(sysregs_save_at_el1)
	mrs	x9, spsr_el1
	mrs	x10, elr_el1
	stp	x9, x10, [x0, #CTX_SPSR_EL1]

	mrs	x15, sctlr_el1
	mrs	x16, tcr_el1
	stp	x15, x16, [x0, #CTX_SCTLR_EL1]

	mrs	x17, cpacr_el1
	mrs	x9, csselr_el1
	stp	x17, x9, [x0, #CTX_CPACR_EL1]

	mrs	x10, sp_el1
	mrs	x11, esr_el1
	stp	x10, x11, [x0, #CTX_SP_EL1]

	mrs	x12, ttbr0_el1
	mrs	x13, ttbr1_el1
	stp	x12, x13, [x0, #CTX_TTBR0_EL1]

	mrs	x14, mair_el1
	mrs	x15, amair_el1
	stp	x14, x15, [x0, #CTX_MAIR_EL1]

	mrs	x16, actlr_el1
	mrs	x17, tpidr_el1
	stp	x16, x17, [x0, #CTX_ACTLR_EL1]

	mrs	x9, tpidr_el0
	mrs	x10, tpidrro_el0
	stp	x9, x10, [x0, #CTX_TPIDR_EL0]

	mrs	x13, par_el1
	mrs	x14, far_el12
	stp	x13, x14, [x0, #CTX_PAR_EL1]

	mrs	x15, afsr0_el1
	mrs	x16, afsr1_el1
	stp	x15, x16, [x0, #CTX_AFSR0_EL1]

	mrs	x17, contextidr_el1
	mrs	x9, vbar_el1
	stp	x17, x9, [x0, #CTX_CONTEXTIDR_EL1]

	/* Save NS timer registers if the build has instructed so */
#if NS_TIMER_SWITCH
	mrs	x10, cntp_ctl_el0
	mrs	x11, cntp_cval_el0
	stp	x10, x11, [x0, #CTX_CNTP_CTL_EL0]

	mrs	x12, cntv_ctl_el0
	mrs	x13, cntv_cval_el0
	stp	x12, x13, [x0, #CTX_CNTV_CTL_EL0]

	mrs	x14, cntkctl_el1
	str	x14, [x0, #CTX_CNTKCTL_EL1]
#endif

	ret
END_FUNC(sysregs_save_at_el1)

BEGIN_FUNC(sysregs_restore_at_el1)
	ldp	x9, x10, [x0, #CTX_SPSR_EL1]
	msr	spsr_el1, x9
	msr	elr_el1, x10

	ldp	x15, x16, [x0, #CTX_SCTLR_EL1]
	msr	sctlr_el1, x15
	msr	tcr_el1, x16

	ldp	x17, x9, [x0, #CTX_CPACR_EL1]
	msr	cpacr_el1, x17
	msr	csselr_el1, x9

	ldp	x10, x11, [x0, #CTX_SP_EL1]
	msr	sp_el1, x10
	msr	esr_el1, x11

	ldp	x12, x13, [x0, #CTX_TTBR0_EL1]
	msr	ttbr0_el1, x12
	msr	ttbr1_el1, x13

	ldp	x14, x15, [x0, #CTX_MAIR_EL1]
	msr	mair_el1, x14
	msr	amair_el1, x15

	ldp 	x16, x17, [x0, #CTX_ACTLR_EL1]
	msr	actlr_el1, x16
	msr	tpidr_el1, x17

	ldp	x9, x10, [x0, #CTX_TPIDR_EL0]
	msr	tpidr_el0, x9
	msr	tpidrro_el0, x10

	ldp	x13, x14, [x0, #CTX_PAR_EL1]
	msr	par_el1, x13
	msr	far_el1, x14

	ldp	x15, x16, [x0, #CTX_AFSR0_EL1]
	msr	afsr0_el1, x15
	msr	afsr1_el1, x16

	ldp	x17, x9, [x0, #CTX_CONTEXTIDR_EL1]
	msr	contextidr_el1, x17
	msr	vbar_el1, x9

	/* Restore NS timer registers if the build has instructed so */
#if NS_TIMER_SWITCH
	ldp	x10, x11, [x0, #CTX_CNTP_CTL_EL0]
	msr	cntp_ctl_el0, x10
	msr	cntp_cval_el0, x11

	ldp	x12, x13, [x0, #CTX_CNTV_CTL_EL0]
	msr	cntv_ctl_el0, x12
	msr	cntv_cval_el0, x13

	ldr	x14, [x0, #CTX_CNTKCTL_EL1]
	msr	cntkctl_el1, x14
#endif

	/* No explict ISB required here as ERET covers it */
	ret
END_FUNC(sysregs_restore_at_el1)

/* ------------------------------------------------------------------
 * The following function is used to save and restore all the general
 * purpose and ARMv8.3-PAuth (if enabled) registers.
 * It also checks if Secure Cycle Counter is not disabled in MDCR_EL3
 * when ARMv8.5-PMU is implemented, and if called from Non-secure
 * state saves PMCR_EL0 and disables Cycle Counter.
 *
 * Ideally we would only save and restore the callee saved registers
 * when a world switch occurs but that type of implementation is more
 * complex. So currently we will always save and restore these
 * registers on entry and exit of EL3.
 * These are not macros to ensure their invocation fits within the 32
 * instructions per exception vector.
 * clobbers: x18
 *
 * Note: x5 stored the value of original sp, restore it before eret.
 *       See el3_common_macros.S for more details.
 * ------------------------------------------------------------------
 */
BEGIN_FUNC(save_gp_pmcr_pauth_regs)
	stp	x0, x1, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X0]
	stp	x2, x3, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X2]
	stp	x4, x5, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X4]
	stp	x6, x7, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X6]
	stp	x8, x9, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X8]
	stp	x10, x11, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X10]
	stp	x12, x13, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X12]
	stp	x14, x15, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X14]
	stp	x16, x17, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X16]
	stp	x18, x19, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X18]
	stp	x20, x21, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X20]
	stp	x22, x23, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X22]
	stp	x24, x25, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X24]
	stp	x26, x27, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X26]
	stp	x28, x29, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X28]
	mrs	x18, sp_el0
	str	x18, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_SP_EL0]

	ret
END_FUNC(save_gp_pmcr_pauth_regs)

/* ------------------------------------------------------------------
 * This function restores ARMv8.3-PAuth (if enabled) and all general
 * purpose registers except x30 from the CPU context.
 * x30 register must be explicitly restored by the caller.
 * ------------------------------------------------------------------
 */
BEGIN_FUNC(restore_gp_pmcr_pauth_regs)
	ldp	x0, x1, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X0]
	ldp	x2, x3, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X2]
	ldp	x4, x5, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X4]
	ldp	x6, x7, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X6]
	ldp	x8, x9, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X8]
	ldp	x10, x11, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X10]
	ldp	x12, x13, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X12]
	ldp	x14, x15, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X14]
	ldp	x16, x17, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X16]
	ldp	x18, x19, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X18]
	ldp	x20, x21, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X20]
	ldp	x22, x23, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X22]
	ldp	x24, x25, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X24]
	ldp	x26, x27, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X26]
	ldr	x28, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_SP_EL0]
	msr	sp_el0, x28
	ldp	x28, x29, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X28]
	ret
END_FUNC(restore_gp_pmcr_pauth_regs)

/* ------------------------------------------------------------------
 * This routine assumes that the SP_EL3 is pointing to a valid
 * context structure from where the gp regs and other special
 * registers can be retrieved.
 * ------------------------------------------------------------------
 */
BEGIN_FUNC(el3_exit)
	/* ----------------------------------------------------------
	 * Save the current SP_EL0 i.e. the EL3 runtime stack which
	 * will be used for handling the next SMC.
	 * Then switch to SP_EL3.
	 * ----------------------------------------------------------
	 */
	mov	x17, sp
	msr	spsel, #MODE_SP_ELX
	str	x17, [sp, #CTX_EL3STATE_OFFSET + CTX_RUNTIME_SP]

	/* ----------------------------------------------------------
	 * Restore SPSR_EL3, ELR_EL3 and SCR_EL3 prior to ERET
	 * Note: We already restore elr_el2 and spsr_el2 in tianium_smc_handler
	 * ----------------------------------------------------------
	 */
	// ldr	x18, [sp, #CTX_EL3STATE_OFFSET + CTX_SCR_EL3]
	// ldp	x16, x17, [sp, #CTX_EL3STATE_OFFSET + CTX_SPSR_EL3]
	// msr	spsr_el2, x16
	// msr	elr_el2, x17

	restore_ptw_el1_sys_regs

	mrs x17, spsr_el2
	tst x17, #(1 << 3)  /* Test spsr_el2.M[3] */
	b.eq ret_to_svisor

	/* ----------------------------------------------------------
	 * Restore general purpose (including x30), PMCR_EL0 and
	 * ARMv8.3-PAuth registers.
	 * Exit EL3 via ERET to a lower exception level.
 	 * ----------------------------------------------------------
 	 */
ret_to_nvisor:
	bl	restore_gp_pmcr_pauth_regs
	ldr	x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
	dsb	sy

	/* restore before return to n-visor */
	el3_restore_sp

	exception_return

ret_to_svisor:
	bl	restore_gp_pmcr_pauth_regs
	ldr	x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
	dsb	sy

	exception_return

END_FUNC(el3_exit)

/* x0: context */
BEGIN_FUNC(cm_set_next_context)
	msr spsel, #MODE_SP_ELX
	mov sp, x0
	msr spsel, #MODE_SP_EL0

	ret
END_FUNC(cm_set_next_context)

/*
 * x0 == 0 - secure; 1 - non-secure
 */
BEGIN_FUNC(cm_get_next_context)
	__get_core_id x1, x2, x3 /* x1 = core_id */
	ldr x2, =titanium_secure_context
	cmp x0, #0
	b.eq 1f
	ldr x2, =titanium_non_secure_context
1:
	mov x3, #TITANIUM_CONTEXT_SIZE
	umaddl x0, w1, w3, x2 /* x0 = &titanium_(non_)secure_context[core_id] */
	add x0, x0, #CPU_CONTEXT_OFFSET

	ret
END_FUNC(cm_get_next_context)

	.popsection
