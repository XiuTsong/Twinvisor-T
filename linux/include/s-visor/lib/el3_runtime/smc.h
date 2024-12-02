/*
 * @Description: Use hvc to emulate smc
 * @Date: 2024-11-11 15:44:16
 */
/*
 * @Description: Emulate smc
 * @Date: 2024-11-11 15:44:16
 */

#ifndef __SVISOR_SMC_H__
#define __SVISOR_SMC_H__

#include <s-visor/el3/titanium_private.h>

#ifdef __ASSEMBLER__

#include <s-visor/lib/el3_runtime/context_macros.S>

/* s-visor can call hvc directly */
.macro __smc imm
	hvc \imm
.endm

/* FIXME: get core id can be optimized */
.macro ____before_nvisor_smc
	cm_save_gp_ctx_secure
	mov x0, #1
	bl cm_el2_eret_state_save
	mrs x1, vbar_el2
	mrs x2, daif
	stp x1, x2, [sp, #-16]!
	cm_get_context_ns
	mov x6, x0
	msr daifset, #2
	ldr x0, =runtime_exceptions
	msr vbar_el2, x0
.endm

.macro ____after_nvisor_smc
	ldp x1, x2, [sp], #16
	msr vbar_el2, x1
	msr daif, x2
	mov x0, #1
	bl cm_el2_eret_state_restore
	cm_restore_gp_ctx_secure
.endm

.macro __nvisor_smc imm
	____before_nvisor_smc
	hvc \imm
	____after_nvisor_smc
.endm

#else

extern char runtime_exceptions[];

struct smc_local_context {
	unsigned long vbar_el2;
	unsigned long flags;
	unsigned long sp_el3;
};

void __before_nvisor_smc(struct smc_local_context *ctx);
void __after_nvisor_smc(struct smc_local_context *ctx);

#define __nvisor_smc_common(imm, statement, ctx) {\
	__before_nvisor_smc(&ctx); \
	statement;  \
	asm volatile("hvc %0\n" : : "I" (imm)); \
	__after_nvisor_smc(&ctx); \
}

#define nvisor_smc_common(imm, statement) {\
	struct smc_local_context ctx; \
	__nvisor_smc_common(imm, statement, ctx); \
}

#define __arg0() { \
	/* Save non-secure context in x6. See el3_common_macor.S */ \
	asm volatile("mov x6, %0\n" : : "r" (ctx.sp_el3) : "x6"); \
}

#define __arg1(x1)	{ \
	asm volatile("mov x1, %0\n" : : "r" (x1) : "x1"); \
	__arg0(); \
}

#define __arg2(x1, x2) { \
	asm volatile("mov x2, %0\n" : : "r" (x2) : "x2"); \
	__arg1(x1); \
}

#define __arg3(x1, x2, x3) { \
	asm volatile("mov x3, %0\n" : : "r" (x3) : "x3"); \
	__arg2(x1, x2); \
}

#define nvisor_smc(imm) {\
	nvisor_smc_common(imm, __arg0()); \
}

#define nvisor_smc_1(imm, x1_val) { \
	nvisor_smc_common(imm, __arg1(x1_val)); \
}

#define nvisor_smc_2(imm, x1_val, x2_val) { \
	nvisor_smc_common(imm, __arg2(x1_val, x2_val)); \
}

#define nvisor_smc_3(imm, x1_val, x2_val, x3_val) { \
	nvisor_smc_common(imm, __arg3(x1_val, x2_val, x3_val)); \
}

#define svisor_smc(imm) {\
	asm volatile("hvc %0\n" : : "I" (imm)); \
}

#endif /* __ASSEMBLER__ */

#endif /* __SVISOR_SMC_H__ */
