/*
 * @Description: titanium kvm_relay.c
 * @Date: 2024-11-14 14:51:50
 */

#include <asm/page-def.h>
#include <asm/sysreg.h>
#include <linux/errno.h>
#include <linux/string.h>

#include <s-visor/s-visor.h>
#include <s-visor/common/list.h>
#include <s-visor/common/macro.h>
#include <s-visor/lib/el3_runtime/smccc.h>
#include <s-visor/lib/stdio.h>
#include <s-visor/sched/smp.h>
#include <s-visor/mm/mm.h>
#include <s-visor/mm/buddy_allocator.h>
#include <s-visor/mm/page_allocator.h>
#include <s-visor/mm/sec_mmu.h>
#include <s-visor/mm/stage2_mmu.h>
#include <s-visor/virt/vcpu.h>
#include <s-visor/virt/vm.h>
#include <s-visor/virt/kvm_host_common.h>
#include <s-visor/virt/kvm_relay.h>
#include <s-visor/virt/vmexit_def.h>
#include <s-visor/virt/sec_defs.h>
#include <s-visor/virt/sec_handler.h>
#include <s-visor/virt/sec_emulate.h>

#include <asm/asm-offsets.h>

extern uint64_t* __secure_data titanium_shared_pages;

kvm_smc_req_t * __secure_text get_smc_req_region_sec(unsigned int core_id)
{
	uint64_t *ptr = titanium_shared_pages + core_id * TITANIUM_MAX_SIZE_PER_CORE;
	/* First 32 entries are for guest gp_regs */
	return (kvm_smc_req_t *)(ptr + 32);
}

__secure_text
static int handle_call_gate(struct titanium_state *state, uint16_t imm,
                            uint32_t core_id, uint32_t vcpu_id)
{
	struct titanium_vm *vm = state->current_vm;
	struct sec_shm *shm = get_shared_mem(vm, core_id);
	int exit_id = imm & 0xff;
	int ret = 0;

	if (exit_id != SEL1_SVC) {
		shm->arg1 = get_guest_gp_reg(state, 0);
		shm->exit_reason = exit_id;
	}
	sel1_handle_dispatcher(vm, core_id, vcpu_id);

	return ret;
}

__secure_text
static int check_fault_addess(struct titanium_state *state, unsigned long fault_ipa)
{
	uint64_t elr_el1 = get_guest_sys_reg(state, elr);

	if (((fault_ipa >> 48 != 0xffff) && (fault_ipa >> 48 != 0x0000)) || elr_el1 == 0) {
		elr_el1 = read_sysreg(elr_el1);
		printf_error("Invalid fault_ipa: %lx elr_el1: %llx\n", fault_ipa, elr_el1);
		set_guest_sys_reg(state, elr, elr_el1 - 4 * 4);
		return CALL_GATE_SYNC;
	}

	return GUEST_SYNC;
}

__secure_text
int decode_kvm_vm_exit(struct titanium_state *state, uint32_t core_id, uint32_t vcpu_id)
{
	uint64_t esr_el = get_guest_sys_reg(state, esr);
	uint64_t kvm_exit_reason = ESR_EL_EC(esr_el);
	uint64_t fault_ipa = get_guest_sys_reg(state, far);
	uint64_t fault_ipn = (fault_ipa >> PAGE_SHIFT);

	if (kvm_exit_reason == ESR_ELx_EC_IABT_CUR ||
			kvm_exit_reason == ESR_ELx_EC_DABT_CUR) {
		/* "stage-2" abort here. FIXME: Hard code here */
		if (fault_ipn >= 0x40000 && fault_ipn < 0x80000) {
			state->current_vm->vcpus[vcpu_id]->is_s2pt_violation = 1;
			state->current_vm->vcpus[vcpu_id]->fault_ipn = fault_ipn;
			return HOST_SYNC;
		}

		/* Check if is mmio abort */
		if (emulate_mmio_fault(state, fault_ipa, vcpu_id)) {
			return HOST_SYNC;
		}
	}
	else if (kvm_exit_reason == ESR_ELx_EC_SVC64) {
		/* Origninal guest-linux does not use svc in EL1 */
		uint16_t imm = esr_el & 0xffff;;
		if (imm == SEL1_SYS64) {
			emulate_access_trap(state, core_id, vcpu_id);
			return HOST_SYNC;
		}
		if (imm > 0) {
			handle_call_gate(state, imm, core_id, vcpu_id);
			return CALL_GATE_SYNC;
		}
		/* guest-linux call "SMCCC svc" */
		return HOST_SYNC;
	}

	return check_fault_addess(state, fault_ipa);
}
