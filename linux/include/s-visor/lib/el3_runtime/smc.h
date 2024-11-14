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

#define nvisor_smc(imm) \
		do { \
			struct smc_local_context ctx; \
			__before_nvisor_smc(&ctx); \
			/* Save non-secure context in x6. See el3_common_macor.S */ \
			asm volatile("mov x6, %0\n" : : "r" (ctx.sp_el3) : "x6"); \
			asm volatile("hvc %0\n" : : "I" (imm)); \
			__after_nvisor_smc(&ctx); \
		} while (0)

#define svisor_smc(imm) \
		do { \
			asm volatile("hvc %0\n" : : "I" (imm)); \
		} while(0)

#endif /* __ASSEMBLER__ */

#endif /* __SVISOR_SMC_H__ */
