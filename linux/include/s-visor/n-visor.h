/*
 * Created on 2024/11/05
 */

#ifndef __SVISOR_NVISOR_H__
#define __SVISOR_NVISOR_H__

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
	unsigned long vbar;
	unsigned long mair;
	unsigned long amair;
	unsigned long tcr;
	unsigned long tpidr;
	unsigned long far;
};

typedef unsigned int svisor_vector_isn_t;

struct svisor_handler {
	svisor_vector_isn_t yield_smc_entry;
	svisor_vector_isn_t fast_smc_entry;
	svisor_vector_isn_t kvm_trap_smc_entry;
	svisor_vector_isn_t kvm_shared_memory_register_entry;
	svisor_vector_isn_t kvm_shared_memory_handle_entry;
	svisor_vector_isn_t cpu_on_entry;
	svisor_vector_isn_t cpu_off_entry;
	svisor_vector_isn_t cpu_resume_entry;
	svisor_vector_isn_t cpu_suspend_entry;
	svisor_vector_isn_t fiq_entry;
	svisor_vector_isn_t system_off_entry;
	svisor_vector_isn_t system_reset_entry;
	svisor_vector_isn_t primary_core_entry;
	svisor_vector_isn_t secondary_core_entry;
};

enum secure_state {
	SECURE,
	NON_SECURE
};

struct nvisor_state {
	struct gp_regs gp_regs;
	unsigned long nvisor_sp;
	unsigned long svisor_sp;
	unsigned long hcr_el2;
	unsigned long spsr_el2;
	unsigned long elr_el2;
	unsigned long vbar_el2;
};

#define SMC_IMM_KVM_TO_TITANIUM_TRAP 			0x1
#define SMC_IMM_TITANIUM_TO_KVM_TRAP_SYNC 		0x1
#define SMC_IMM_TITANIUM_TO_KVM_TRAP_IRQ 		0x2
#define SMC_IMM_KVM_TO_TITANIUM_SHARED_MEMORY_REGISTER 	0x10
#define SMC_IMM_KVM_TO_TITANIUM_SHARED_MEMORY_HANDLE 	0x18
#define SMC_IMM_TITANIUM_TO_KVM_SHARED_MEMORY 		0x10
#define SMC_IMM_TITANIUM_TO_KVM_FIXUP_VTTBR 		0x20
#define SMC_IMM_KVM_TO_TITANIUM_PRIMARY 		0x100
#define SMC_IMM_KVM_TO_TITANIUM_SECONDARY 		0x101

void switch_to_svisor(unsigned int smc_imm);
void primary_switch_to_svisor(void);

#endif
