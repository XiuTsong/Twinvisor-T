/*
 * @Description: common defines
 * @Date: 2024-11-08 15:02:53
 */

#ifndef __SVISOR_COMMON_DEFS_H__
#define __SVISOR_COMMON_DEFS_H__

#define SVISOR_PHYSICAL_CORE_NUM 4

#ifndef __ASSEMBLER__

struct gp_regs {
	unsigned long x[30];
	unsigned long lr;
	unsigned long pc;
};

struct sys_regs {
	unsigned long spsr;
	unsigned long elr;
	unsigned long sctlr;
	unsigned long sp;
	unsigned long sp_el0;
	unsigned long esr;
	unsigned long ttbr0;
	unsigned long ttbr1;
	unsigned long vbar;
	unsigned long mair;
	unsigned long amair;
	unsigned long tcr;
	unsigned long tpidr;
	unsigned long far;
};

typedef unsigned long u_register_t;

#endif

#endif