/*
 * @Description: el3 context switch
 * @Date: 2024-11-11 10:48:56
 */

#ifndef __SVISOR_CONTEXT_H__
#define __SVISOR_CONTEXT_H__

#include <asm/asm-offsets.h>
#include <s-visor/lib/utils_def.h>

/*******************************************************************************
 * Constants that allow assembler code to access members of and the 'gp_regs'
 * structure at their correct offsets.
 ******************************************************************************/
// #define CTX_GPREGS_OFFSET	U(0x0)
#define CTX_GPREG_X0		U(0x0)
#define CTX_GPREG_X1		U(0x8)
#define CTX_GPREG_X2		U(0x10)
#define CTX_GPREG_X3		U(0x18)
#define CTX_GPREG_X4		U(0x20)
#define CTX_GPREG_X5		U(0x28)
#define CTX_GPREG_X6		U(0x30)
#define CTX_GPREG_X7		U(0x38)
#define CTX_GPREG_X8		U(0x40)
#define CTX_GPREG_X9		U(0x48)
#define CTX_GPREG_X10		U(0x50)
#define CTX_GPREG_X11		U(0x58)
#define CTX_GPREG_X12		U(0x60)
#define CTX_GPREG_X13		U(0x68)
#define CTX_GPREG_X14		U(0x70)
#define CTX_GPREG_X15		U(0x78)
#define CTX_GPREG_X16		U(0x80)
#define CTX_GPREG_X17		U(0x88)
#define CTX_GPREG_X18		U(0x90)
#define CTX_GPREG_X19		U(0x98)
#define CTX_GPREG_X20		U(0xa0)
#define CTX_GPREG_X21		U(0xa8)
#define CTX_GPREG_X22		U(0xb0)
#define CTX_GPREG_X23		U(0xb8)
#define CTX_GPREG_X24		U(0xc0)
#define CTX_GPREG_X25		U(0xc8)
#define CTX_GPREG_X26		U(0xd0)
#define CTX_GPREG_X27		U(0xd8)
#define CTX_GPREG_X28		U(0xe0)
#define CTX_GPREG_X29		U(0xe8)
#define CTX_GPREG_LR		U(0xf0)
#define CTX_GPREG_SP_EL0	U(0xf8)
#define CTX_GPREGS_END		U(0x100)

/*******************************************************************************
 * Constants that allow assembler code to access members of and the 'el3_state'
 * structure at their correct offsets. Note that some of the registers are only
 * 32-bits wide but are stored as 64-bit values for convenience
 ******************************************************************************/
// #define CTX_EL3STATE_OFFSET	(CTX_GPREGS_OFFSET + CTX_GPREGS_END)
#define CTX_SCR_EL3		U(0x0)
#define CTX_ESR_EL3		U(0x8)
#define CTX_RUNTIME_SP		U(0x10)
#define CTX_SPSR_EL3		U(0x18)
#define CTX_ELR_EL3		U(0x20)
#define CTX_PMCR_EL0		U(0x28)
#define CTX_IS_IN_EL3		U(0x30)
#define CTX_EL3STATE_END	U(0x40) /* Align to the next 16 byte boundary */

/*******************************************************************************
 * Constants that allow assembler code to access members of and the
 * 'el1_sys_regs' structure at their correct offsets. Note that some of the
 * registers are only 32-bits wide but are stored as 64-bit values for
 * convenience
 ******************************************************************************/
// #define CTX_EL1_SYSREGS_OFFSET	(CTX_EL3STATE_OFFSET + CTX_EL3STATE_END)
#define CTX_SPSR_EL1		U(0x0)
#define CTX_ELR_EL1		U(0x8)
#define CTX_SCTLR_EL1		U(0x10)
#define CTX_TCR_EL1		U(0x18)
#define CTX_CPACR_EL1		U(0x20)
#define CTX_CSSELR_EL1		U(0x28)
#define CTX_SP_EL1		U(0x30)
#define CTX_ESR_EL1		U(0x38)
#define CTX_TTBR0_EL1		U(0x40)
#define CTX_TTBR1_EL1		U(0x48)
#define CTX_MAIR_EL1		U(0x50)
#define CTX_AMAIR_EL1		U(0x58)
#define CTX_ACTLR_EL1		U(0x60)
#define CTX_TPIDR_EL1		U(0x68)
#define CTX_TPIDR_EL0		U(0x70)
#define CTX_TPIDRRO_EL0		U(0x78)
#define CTX_PAR_EL1		U(0x80)
#define CTX_FAR_EL1		U(0x88)
#define CTX_AFSR0_EL1		U(0x90)
#define CTX_AFSR1_EL1		U(0x98)
#define CTX_CONTEXTIDR_EL1	U(0xa0)
#define CTX_VBAR_EL1		U(0xa8)
#define CTX_EL1_SYSREGS_END U(0xb0) /* Align to the next 16 byte boundary */

/*
 * If the timer registers aren't saved and restored, we don't have to reserve
 * space for them in the context
 */
#if NS_TIMER_SWITCH
#define CTX_CNTP_CTL_EL0	(CTX_AARCH32_END + U(0x0))
#define CTX_CNTP_CVAL_EL0	(CTX_AARCH32_END + U(0x8))
#define CTX_CNTV_CTL_EL0	(CTX_AARCH32_END + U(0x10))
#define CTX_CNTV_CVAL_EL0	(CTX_AARCH32_END + U(0x18))
#define CTX_CNTKCTL_EL1		(CTX_AARCH32_END + U(0x20))
#define CTX_TIMER_SYSREGS_END	(CTX_AARCH32_END + U(0x30)) /* Align to the next 16 byte boundary */
#else
#define CTX_TIMER_SYSREGS_END	CTX_AARCH32_END
#endif /* NS_TIMER_SWITCH */

/*
 * End of system registers.
 */

/*******************************************************************************
 * Titanium: el2_sysregs_ctx
 ******************************************************************************/
// #define CTX_EL2_SYSREGS_OFFSET	(CTX_EL1SRE_OFFSET + CTX_EL1SRE_END)
#define CTX_SPSR_EL2		U(0x0)
#define CTX_ELR_EL2		    U(0x8)
#define CTX_SCTLR_EL2		U(0x10)
#define CTX_ACTLR_EL2		U(0x18)
#define CTX_VSTTBR_EL2		U(0x20)
#define CTX_VSTCR_EL2		U(0x28)
#define CTX_VTTBR_EL2		U(0x30)
#define CTX_VTCR_EL2		U(0x38)
#define CTX_SP_EL2		    U(0x40)
#define CTX_ESR_EL2		    U(0x48)
#define CTX_TTBR0_EL2		U(0x50)
#define CTX_TTBR1_EL2		U(0x58)
#define CTX_MAIR_EL2		U(0x60)
#define CTX_AMAIR_EL2		U(0x68)
#define CTX_TCR_EL2		    U(0x70)
#define CTX_TPIDR_SEL0		U(0x78)
#define CTX_TPIDR_SEL1		U(0x80)
#define CTX_TPIDR_EL2		U(0x88)
#define CTX_PAR_SEL1	   	U(0x90)
#define CTX_HPFAR_EL2	   	U(0x98)
#define CTX_FAR_EL2		    U(0xa0)
#define CTX_AFSR0_EL2		U(0xa8)
#define CTX_AFSR1_EL2		U(0xb0)
#define CTX_CONTEXTIDR_EL2	U(0xb8)
#define CTX_VBAR_EL2		U(0xc0)
#define CTX_HCR_EL2         U(0xc8)
#define CTX_CNTHCTL_EL2		U(0xd0)
#define CTX_CNTHP_CTL_EL2         U(0xd8)
#define CTX_CNTHP_CVAL_EL2         U(0xe0)
#define CTX_CNTHP_TVAL_EL2         U(0xe8)
#define CTX_CNTHV_CTL_EL2         U(0xf0)
#define CTX_CNTHV_CVAL_EL2         U(0xf8)
#define CTX_CNTHV_TVAL_EL2         U(0x100)
#define CTX_MDCR_EL2			   U(0x108)
#define CTX_EL2_SYSREGS_END U(0x110)

#ifndef __ASSEMBLER__

#include <s-visor/lib/stdint.h>
#include <s-visor/lib/cassert.h>

/*
 * Common constants to help define the 'cpu_context' structure and its
 * members below.
 */
#define DWORD_SHIFT		U(3)
#define DEFINE_REG_STRUCT(name, num_regs)	\
	typedef struct name {			\
		uint64_t ctx_regs[num_regs];	\
	}  __aligned(16) name##_t

/* Constants to determine the size of individual context structures */
#define CTX_GPREG_ALL		(CTX_GPREGS_END >> DWORD_SHIFT)
#define CTX_EL1_SYSREGS_ALL	(CTX_EL1_SYSREGS_END >> DWORD_SHIFT)
#define CTX_EL2_SYSREGS_ALL	(CTX_EL2_SYSREGS_END >> DWORD_SHIFT)
#define CTX_EL3STATE_ALL	(CTX_EL3STATE_END >> DWORD_SHIFT)

/*
 * AArch64 general purpose register context structure. Usually x0-x18,
 * lr are saved as the compiler is expected to preserve the remaining
 * callee saved registers if used by the C runtime and the assembler
 * does not touch the remaining. But in case of world switch during
 * exception handling, we need to save the callee registers too.
 */
typedef struct _gp_regs {
	uint64_t ctx_regs[CTX_GPREG_ALL];
}  __aligned(16) gp_regs_t;

/*
 * AArch64 EL1 system register context structure for preserving the
 * architectural state during world switches.
 */
DEFINE_REG_STRUCT(el1_sysregs, CTX_EL1_SYSREGS_ALL);
DEFINE_REG_STRUCT(el2_sysregs, CTX_EL2_SYSREGS_ALL);

/*
 * Miscellaneous registers used by EL3 firmware to maintain its state
 * across exception entries and exits
 */
DEFINE_REG_STRUCT(el3_state, CTX_EL3STATE_ALL);

/*
 * Macros to access members of any of the above structures using their
 * offsets
 */
#define read_ctx_reg(ctx, offset)	((ctx)->ctx_regs[(offset) >> DWORD_SHIFT])
#define write_ctx_reg(ctx, offset, val)	(((ctx)->ctx_regs[(offset) >> DWORD_SHIFT]) \
					 = (uint64_t) (val))

typedef struct __cpu_context {
	gp_regs_t gpregs_ctx;
	el3_state_t el3_state_ctx;
	el1_sysregs_t el1_sysregs_ctx;
	el2_sysregs_t el2_sysregs_ctx;
} cpu_context_t;

/* Macros to access members of the 'cpu_context_t' structure */
#define get_el3state_ctx(h)	(&((cpu_context_t *) h)->el3_state_ctx)
#define get_el1_sysregs_ctx(h)	(&((cpu_context_t *) h)->el1_sysregs_ctx)
#define get_el2_sysregs_ctx(h)	(&((cpu_context_t *) h)->el2_sysregs_ctx)
#define get_gpregs_ctx(h)	(&((cpu_context_t *) h)->gpregs_ctx)

/*******************************************************************************
 * Function prototypes
 ******************************************************************************/
void el1_sysregs_context_save(el1_sysregs_t *regs);
void el1_sysregs_context_restore(el1_sysregs_t *regs);

void el2_sysregs_context_save(el2_sysregs_t *regs);
void el2_sysregs_context_restore(el2_sysregs_t *regs);
void el2_sysregs_context_save_host_only(el2_sysregs_t *regs);
void el2_sysregs_context_restore_host_only(el2_sysregs_t *regs);

#endif /* __ASSEMBLER__ */

#endif
