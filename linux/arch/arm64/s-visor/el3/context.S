#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/kvm_arm.h>

#include <s-visor/virt/registers-asm.h>
#include <s-visor/arch/arm64/misc.h>
#include <s-visor/lib/el3_runtime/context.h>

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
