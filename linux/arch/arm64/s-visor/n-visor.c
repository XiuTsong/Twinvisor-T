/*
 * Created on 2024/11/05
 */

#include <asm/sections.h>
#include <asm/sysreg.h>
#include <asm/memory.h>
#include <asm/kvm_asm.h>
#include <linux/smp.h>

#include <s-visor/n-visor.h>
#include <s-visor/s-visor.h>
#include <s-visor/sched/smp.h>
#include <s-visor/arch/arm64/arch.h>
#include <s-visor/mm/mmu.h>

#pragma GCC optimize("O0")

struct nvisor_state global_nvisor_states[SVISOR_PHYSICAL_CORE_NUM];

/* Address of the entrypoint vector table in s-visor. */
struct svisor_handler *svisor_handler_table;

struct nvisor_state *get_global_nvisor_state(unsigned int core_id)
{
	return &global_nvisor_states[core_id];
}

extern unsigned long __switch_to_svisor(struct nvisor_state *state);

static void save_nvisor_state(struct nvisor_state *state)
{
	state->spsr_el2 = read_sysreg(spsr_el2);
	state->elr_el2 = read_sysreg(elr_el2);
}

static void restore_nvisor_state(struct nvisor_state *state)
{
	write_sysreg(state->spsr_el2, spsr_el2);
	write_sysreg(state->elr_el2, elr_el2);
}

static inline void set_entry_context(enum secure_state secure_state,
				     				 unsigned long entrypoint)
{
	if (secure_state == SECURE) {
		write_sysreg(entrypoint, elr_el2);
		write_sysreg(SPSR_64(MODE_EL1, MODE_SP_ELX,
				     DISABLE_ALL_EXCEPTIONS),
			     	 spsr_el2);
	}
}

extern char __svisor_early_stack[];

static inline void set_early_stack(struct nvisor_state *state)
{
	state->svisor_sp = (unsigned long)__svisor_early_stack;
}

static void handle_switch_from_nvisor(struct nvisor_state *state, unsigned int imm)
{
	switch (imm) {
	case SMC_IMM_KVM_TO_TITANIUM_PRIMARY:
		set_early_stack(state);
		set_entry_context(SECURE,
						(unsigned long)&svisor_handler_table->primary_core_entry);
		break;
	default:
		break;
	}
}

static void setup_svisor_sysregs(void)
{
	write_sysreg(read_sysreg(sctlr_el2), sctlr_el12);
	write_sysreg(read_sysreg(mair_el2), mair_el12);
	write_sysreg(read_sysreg(tcr_el2), tcr_el12);
}

void switch_to_svisor(unsigned int imm)
{
	unsigned int core_id = smp_processor_id();
	struct nvisor_state *state = get_global_nvisor_state(core_id);

	save_nvisor_state(state);
	handle_switch_from_nvisor(state, imm);

	/* Here we switch to s-visor */
	__switch_to_svisor(state);

	restore_nvisor_state(state);
}

static void setup_svisor_pgtable(void)
{
	write_sysreg(SECURE_PG_DIR_PHYS, ttbr1_el12);
}

void primary_switch_to_svisor(void)
{
	/* The first time we switch to s-visor */
	svisor_handler_table = (struct svisor_handler *)__svisor_handler;

	setup_svisor_sysregs();
	setup_svisor_pgtable();

	switch_to_svisor(SMC_IMM_KVM_TO_TITANIUM_PRIMARY);
}
