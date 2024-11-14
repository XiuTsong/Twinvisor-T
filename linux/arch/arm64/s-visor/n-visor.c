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
#include <s-visor/lib/el3_runtime/context.h>
#include <s-visor/lib/el3_runtime/smc.h>
#include <s-visor/el3/titanium_private.h>
#include <s-visor/el3/context_mgmt.h>
#include <s-visor/el3/runtime_svc.h>
#include <s-visor/el3/ep_info.h>

#ifdef SVISOR_DEBUG
#pragma GCC optimize("O0")
#endif

extern char __el3_runtime_stack[];

static void setup_el3_runtime_stack(void)
{
	uint64_t offset = (smp_processor_id() + 1) * PAGE_SIZE;
	cpu_context_t *cpu_ctx = cm_get_next_context(NON_SECURE);
	el3_state_t *el3_state = get_el3state_ctx(cpu_ctx);

	write_ctx_reg(el3_state, CTX_RUNTIME_SP, (unsigned long)__el3_runtime_stack + offset);
}

static void setup_svisor_sysregs(void)
{
	cpu_context_t *cpu_ctx = cm_get_next_context(SECURE);
	el1_sysregs_t *el1_sys_regs = get_el1_sysregs_ctx(cpu_ctx);

	write_ctx_reg(el1_sys_regs, CTX_SCTLR_EL1, read_sysreg(sctlr_el2));
	write_ctx_reg(el1_sys_regs, CTX_MAIR_EL1, read_sysreg(mair_el2));
	write_ctx_reg(el1_sys_regs, CTX_TCR_EL1, read_sysreg(tcr_el2));
}

static void setup_svisor_pgtable(void)
{
	cpu_context_t *cpu_ctx = cm_get_next_context(SECURE);
	el1_sysregs_t *el1_sys_regs = get_el1_sysregs_ctx(cpu_ctx);

	write_ctx_reg(el1_sys_regs, CTX_TTBR1_EL1, SECURE_PG_DIR_PHYS);
}

extern void el3_context_init(void);

static void setup_el3(bool is_primary)
{
	if (is_primary) {
		el3_context_init();
		runtime_svc_init();
	}
	setup_el3_runtime_stack();
}

static void setup_titanium(void)
{
	setup_svisor_sysregs();
	setup_svisor_pgtable();
}

void primary_switch_to_svisor(void)
{
	/* The first time we switch to s-visor */
	setup_el3(true);
	setup_titanium();

	nvisor_smc(SMC_IMM_KVM_TO_TITANIUM_PRIMARY);
}

void secondary_switch_to_svisor(void)
{
	/* The first time we switch to s-visor */
	setup_el3(false);
	setup_titanium();

	nvisor_smc(SMC_IMM_KVM_TO_TITANIUM_SECONDARY);
}
