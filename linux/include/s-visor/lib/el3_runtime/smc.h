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

#ifdef __ASSEMBLER__

/* s-visor can call hvc directly */
.macro __smc imm
	hvc \imm
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
