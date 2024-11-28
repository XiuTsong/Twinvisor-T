/*
 * @Description: trusted-firmware-a titanium_main.c
 * @Date: 2024-11-11 19:31:49
 */

#include <asm/sysreg.h>
#include <asm/kvm_arm.h>
#include <linux/string.h>

#include <s-visor/s-visor.h>
#include <s-visor/lib/stdint.h>
#include <s-visor/lib/utils_def.h>
#include <s-visor/lib/assert.h>
#include <s-visor/lib/el3_runtime/context.h>
#include <s-visor/lib/el3_runtime/smccc.h>
#include <s-visor/lib/el3_runtime/smccc_helper.h>
#include <s-visor/lib/el3_runtime/panic.h>
#include <s-visor/el3/platform.h>
#include <s-visor/el3/runtime_svc.h>
#include <s-visor/el3/section.h>
#include <s-visor/el3/titanium_private.h>
#include <s-visor/el3/context_mgmt.h>
#include <s-visor/el3/ep_info.h>
#include <s-visor/el3/teesmc_titanium.h>
#include <s-visor/virt/vmexit_def.h>
#include <s-visor/arch/arm64/arch.h>

#ifdef SVISOR_DEBUG
#pragma GCC optimize("O0")
#endif

/*******************************************************************************
 * Address of the entrypoint vector table in TITANIUM. It is
 * initialised once on the primary core after a cold boot.
 ******************************************************************************/
extern char __svisor_handler[];
struct titanium_vectors __el3_data *titanium_vector_table;

extern char __kvm_hyp_vector[];
#define NVISOR_HYP_VECTOR_BASE ((unsigned long)__kvm_hyp_vector)

/*******************************************************************************
 * Array to keep track of per-cpu TITANIUM state
 ******************************************************************************/
titanium_context_t __el3_data titanium_secure_context[TITANIUM_CORE_COUNT];
titanium_context_t __el3_data titanium_non_secure_context[TITANIUM_CORE_COUNT];
uint32_t __el3_data titanium_rw;
static bool __el3_data is_fixup_vttbr[TITANIUM_CORE_COUNT];
static u_register_t __el3_data fixup_vttbr_elr_el3[TITANIUM_CORE_COUNT];

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static void __el3_text pass_el2_return_state_to_el1(titanium_context_t *titanium_ctx)
{
	uint64_t elr_el2 = read_sysreg(elr_el2);
	uint64_t spsr_el2 = read_sysreg(spsr_el2);

	/* Pass ELR_EL2 to ELR_EL1 */
	write_ctx_reg(get_el1_sysregs_ctx(&titanium_ctx->cpu_ctx),
				  CTX_ELR_EL1, elr_el2);

	/* Pass SPSR_EL2 to SPSR_EL1 */
	write_ctx_reg(get_el1_sysregs_ctx(&titanium_ctx->cpu_ctx),
				  CTX_SPSR_EL1, spsr_el2);
}

static void __el3_text pass_el1_return_state_to_el2(titanium_context_t *titanium_ctx)
{
	uint64_t elr_el1 = read_sysreg(elr_el12);
	uint64_t spsr_el1 = read_sysreg(spsr_el12);

	/* Pass ELR_EL1 to ELR_EL2 */
	write_sysreg(elr_el1, elr_el2);

	/* Pass SPSR_EL1 to SPSR_EL2 */
	write_sysreg(spsr_el1, spsr_el2);
}

static void __el3_text pass_el1_fault_state_to_el2(titanium_context_t *titanium_ctx)
{
	unsigned long esr_el1 = read_sysreg(esr_el12);
	unsigned long far_el1 = read_sysreg(far_el12);
	unsigned long hpfar_el2 = 0;
	uint64_t kvm_exit_reason = ESR_EL_EC(esr_el1);

	/* Pass ESR_EL1 to ESR_EL2 */
	if (kvm_exit_reason == ESR_ELx_EC_IABT_CUR ||
		kvm_exit_reason == ESR_ELx_EC_DABT_CUR) {
		/* Change IABT/DABT_CUR to IABT/DABT_LOW.
		 * Clear EC bits[0], or ESR_EL bits[26]
		 */
		esr_el1 &= (~BIT(26));
	}
	if (kvm_exit_reason == ESR_ELx_EC_SVC64) {
		/* Change SVC64 to HVC64 */
		esr_el1 &= (~BIT(26));
		esr_el1 |= (BIT(27));
	}
	write_sysreg(esr_el1, esr_el2);

	/* Pass FAR_EL1 to HPFAR_EL2 */
	hpfar_el2 = (far_el1 >> 12) << 4;
	write_sysreg(hpfar_el2, hpfar_el2);

	/* Pass FAR_EL1 to FAR_EL2
	 * FIXME: Does nvisor only need low 12-bit of FAR_EL2?
	 */
	write_sysreg(far_el1, far_el2);
}

/*******************************************************************************
 * TITANIUM Dispatcher setup. The TITANIUM finds out the TITANIUM entrypoint and type
 * (aarch32/aarch64) if not already known and initialises the context for entry
 * into TITANIUM for its initialization.
 ******************************************************************************/

void __el3_text el3_context_init(void)
{
	memset(titanium_secure_context, 0, sizeof(titanium_secure_context));
	memset(titanium_non_secure_context, 0, sizeof(titanium_non_secure_context));
}

static int32_t __el3_text titanium_setup(void)
{
	uint32_t idx;

	for (idx = 0U; idx < TITANIUM_CORE_COUNT; idx++) {
		cm_set_context_by_index(idx, &titanium_secure_context[idx].cpu_ctx, SECURE);
		cm_set_context_by_index(idx, &titanium_non_secure_context[idx].cpu_ctx, NON_SECURE);
	}
	titanium_vector_table = (struct titanium_vectors *)__svisor_handler;

	return 0;
}

static uintptr_t __el3_text smc_handle_from_non_secure(void *handle, uint32_t smc_imm)
{
	uint32_t linear_id = plat_my_core_pos();
	titanium_context_t *titanium_ctx = &titanium_secure_context[linear_id];

	/*
	 * This is a fresh request from the non-secure client.
	 * The parameters are in x1 and x2. Figure out which
	 * registers need to be preserved, save the non-secure
	 * state and send the request to the secure payload.
	 */
	assert(handle == cm_get_context(NON_SECURE));
	cm_el1_sysregs_context_save(NON_SECURE);

	if (smc_imm == SMC_IMM_KVM_TO_TITANIUM_TRAP) {
		/*
		 * hvc will change ELR_EL2 and SPSR_EL2 which s-visor will use to enter guest.
		 * Luckily, we have saved the needed value in the ns context before hvc(see smc.c).
		 * Just restore them here.
		 */
		cm_el2_eret_state_restore(NON_SECURE);
		pass_el2_return_state_to_el1(titanium_ctx);
	}

	/*
	 * We are done stashing the non-secure context. Ask the
	 * TITANIUM to do the work now.
	 *
	 * Verify if there is a valid context to use, copy the
	 * operation type and parameters to the secure context
	 * and jump to the fast smc entry point in the secure
	 * payload. Entry into S-EL1 will take place upon exit
	 * from this function.
	 */
	assert(&titanium_ctx->cpu_ctx == cm_get_context(SECURE));

	/* Set appropriate entry for SMC. */
	switch (smc_imm) {
		case SMC_IMM_KVM_TO_TITANIUM_PRIMARY:
			cm_set_elr_spsr_el3(SECURE,
								(uint64_t)&titanium_vector_table->primary_core_entry,
								SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS));
			/*
			 * Pass vp_offset in x1.
			 * See n-visor.c:primary_switch_to_svisor()
			 */
			write_ctx_reg(get_gpregs_ctx(&titanium_ctx->cpu_ctx), CTX_GPREG_X1,
							read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X1));
			break;
		case SMC_IMM_KVM_TO_TITANIUM_SECONDARY:
			cm_set_elr_spsr_el3(SECURE,
								(uint64_t)&titanium_vector_table->secondary_core_entry,
								SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS));
			break;
		case SMC_IMM_KVM_TO_TITANIUM_TRAP:
			if (!is_fixup_vttbr[linear_id]) {
				cm_set_elr_el3(SECURE,
							   (uint64_t)&titanium_vector_table->kvm_trap_smc_entry);
			} else {
				is_fixup_vttbr[linear_id] = 0;
				cm_set_elr_el3(SECURE, fixup_vttbr_elr_el3[linear_id]);
			}
			break;
		case SMC_IMM_KVM_TO_TITANIUM_SHARED_MEMORY_REGISTER:
			cm_set_elr_el3(SECURE,
						   (uint64_t)&titanium_vector_table->kvm_shared_memory_register_entry);
			/*
			 * Pass physical address of shared memory base in x1.
			 * See n-visor.c:primary_switch_to_svisor()
			 */
			write_ctx_reg(get_gpregs_ctx(&titanium_ctx->cpu_ctx), CTX_GPREG_X1,
							read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X1));
			break;
		case SMC_IMM_KVM_TO_TITANIUM_SHARED_MEMORY_HANDLE:
			cm_set_elr_el3(SECURE,
						   (uint64_t)&titanium_vector_table->kvm_shared_memory_handle_entry);
			break;
		default:
			el3_panic();
	}

	/* Restore s-visor el1 system registers */
	cm_el1_sysregs_context_restore(SECURE);

	/* We will return to s-visor, only reserve HCR_EL2.E2H and HCR_EL2.RW */
	write_ctx_reg(get_el2_sysregs_ctx(handle), CTX_HCR_EL2, read_sysreg(hcr_el2));
	write_sysreg(HCR_E2H | HCR_RW, hcr_el2);

	/*
	 * We clear bits in MDCR_EL2 to avoid guest trapping to EL2
	 * when they access to debug and performance related registers.
	 * Only MDCR_EL2.HPMN is preserved now.
	 */
	write_ctx_reg(get_el2_sysregs_ctx(handle), CTX_MDCR_EL2, read_sysreg(mdcr_el2));
	write_sysreg(read_sysreg(mdcr_el2) & MDCR_EL2_HPMN_MASK, mdcr_el2);

	cm_set_next_eret_context(SECURE);
	SMC_RET0(&titanium_ctx->cpu_ctx);
}

static uintptr_t __el3_text smc_handle_from_secure(void *handle, uint32_t smc_imm)
{
	uint32_t linear_id = plat_my_core_pos();
	cpu_context_t *ns_cpu_context;
	titanium_context_t *titanium_ctx = &titanium_secure_context[linear_id];
	uint32_t exit_value = smc_imm - 1;

	cm_el1_sysregs_context_save(SECURE);

	/*
	 * s-vm exit, we need to pass information to n-visor.
	 */
	pass_el1_return_state_to_el2(titanium_ctx);
	pass_el1_fault_state_to_el2(titanium_ctx);

	/*
	 * We will soon change ELR_EL2 and SPSR_EL2 soon for eret.
	 * However, we want to pass their *current* to n-visor.
	 * Save them in ns context and restore them after eret (see smc.c).
	 */
	cm_el2_eret_state_save(NON_SECURE);

	/* Get a reference to the non-secure context */
	ns_cpu_context = cm_get_context(NON_SECURE);
	assert(ns_cpu_context);

	/* Restore non-secure el1 system registers. */
	cm_el1_sysregs_context_restore(NON_SECURE);
	switch (smc_imm) {
		case SMC_IMM_TITANIUM_TO_KVM_TRAP_SYNC:
		case SMC_IMM_TITANIUM_TO_KVM_TRAP_IRQ:
			/* skip the first eight handler */
			cm_set_elr_el3(NON_SECURE,
						   NVISOR_HYP_VECTOR_BASE + (8 + exit_value) * 0x80);
			break;
		case SMC_IMM_TITANIUM_TO_KVM_FIXUP_VTTBR:
			asm volatile("mrs %0, elr_el2" : "=r" (fixup_vttbr_elr_el3[linear_id]));
			is_fixup_vttbr[linear_id] = 1;
			/* Set exit_value the same as SMC_IMM_TITANIUM_TO_KVM_TRAP_SYNC */
			exit_value = 0;
			cm_set_elr_el3(NON_SECURE,
						   NVISOR_HYP_VECTOR_BASE + (8 + exit_value) * 0x80);
			break;
		case SMC_IMM_TITANIUM_TO_KVM_SHARED_MEMORY:
		case SMC_IMM_TITANIUM_TO_KVM_RETURN:
			break;
		default:
			el3_panic();
	}

	/* We will return to n-visor, set the original HCR_EL2. */
	write_sysreg(read_ctx_reg(get_el2_sysregs_ctx(ns_cpu_context), CTX_HCR_EL2), hcr_el2);

	/* Restore MDCR_EL2 */
	write_sysreg(read_ctx_reg(get_el2_sysregs_ctx(ns_cpu_context), CTX_MDCR_EL2), mdcr_el2);

	cm_set_next_eret_context(NON_SECURE);
	SMC_RET0(ns_cpu_context);
}

/*******************************************************************************
 * This function is responsible for handling all SMCs in the Trusted OS/App
 * range from the non-secure state as defined in the SMC Calling Convention
 * Document. It is also responsible for communicating with the Secure
 * payload to delegate work and return results back to the non-secure
 * state. Lastly it will also return any information that TITANIUM needs to do
 * the work assigned to it.
 ******************************************************************************/
static uintptr_t __el3_text
titanium_smc_handler(uint32_t smc_fid,
					 u_register_t x1,
					 u_register_t x2,
					 u_register_t x3,
					 u_register_t x4,
					 void *cookie,
					 void *handle,
					 u_register_t flags)
{
	uint32_t smc_imm = read_sysreg(esr_el2) & 0xffff;

	/*
	 * Determine which security state this SMC originated from
	 */
	if (smc_imm == 0) {
		el3_panic();
	}
	if (is_caller_non_secure(flags)) {
		return smc_handle_from_non_secure(handle, smc_imm);
	}
	return smc_handle_from_secure(handle, smc_imm);
}

/* Define an TITANIUM runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
	titanium_fast,

	OEN_TOS_START,
	OEN_TOS_END,
	SMC_TYPE_FAST,
	titanium_setup,
	titanium_smc_handler
);

/* Define an TITANIUM runtime service descriptor for yielding SMC calls */
DECLARE_RT_SVC(
	titanium_std,

	OEN_TOS_START,
	OEN_TOS_END,
	SMC_TYPE_YIELD,
	NULL,
	titanium_smc_handler
);
