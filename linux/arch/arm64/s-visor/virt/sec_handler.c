/*
 * @Description: titanium sel1_handler.c
 * @Date: 2024-11-25 19:14:33
 */

#include <s-visor/lib/stdint.h>
#include <s-visor/lib/stdio.h>

#include <s-visor/s-visor.h>
#include <s-visor/mm/sec_mmu.h>
#include <s-visor/virt/vm.h>
#include <s-visor/virt/sec_defs.h>
#include <s-visor/virt/sec_handler.h>

#define DEBUG_LEVEL_0
// #define DEBUG_LEVEL_1

__secure_text
static void sel1_handle_wfi(void)
{
	asm volatile("nop\n");
}

__secure_text
static void handle_write_sctlr(struct titanium_vcpu *vcpu, u_register_t val, uint64_t vcpu_id)
{
	struct titanium_state *state = this_core_titanium_state();
#ifdef DEBUG_LEVEL_0
	printf("vcpu_id: %lld catch write sctlr, val: %lx\n", vcpu_id, val);
#endif
	if (!(val & 0x1)) {
		printf("Guest try to disable mmu!\n");
		return;
	}
	set_guest_sys_reg(state, sctlr, val);
	vcpu->is_vm_mmu_enable = 1;
}

__secure_text
static void handle_write_vbar(struct titanium_vm *vm, u_register_t val, uint64_t vcpu_id)
{
#ifdef DEBUG_LEVEL_0
	printf("vcpu_id: %lld catch write vbar_el1, val: %lx\n", vcpu_id, val);
#endif
	vm->guest_vector = val;
}

__secure_text
static void handle_write_tcr(u_register_t val, uint64_t vcpu_id)
{
#ifdef DEBUG_LEVEL_0
	printf("vcpu_id: %lld catch write tcr, val: %lx\n", vcpu_id, val);
#endif
}

__secure_text
static void handle_write_mair(u_register_t val, uint64_t vcpu_id)
{
#ifdef DEBUG_LEVEL_0
	printf("vcpu_id: %lld catch write mair, val: %lx\n", vcpu_id, val);
#endif
}

__secure_text
static void handle_read_mpidr(struct titanium_vcpu *vcpu, struct sec_shm *shared_mem)
{
	shared_mem->ret_val = vcpu->vmpidr_el2;
}

__secure_text
static void handle_read_midr(struct titanium_vm *vm, struct sec_shm *shared_mem)
{
	shared_mem->ret_val = vm->vpidr_el2;
}

__secure_text
static void handle_init_swiotlb(paddr_t start_ipa, paddr_t end_ipa, uint64_t vcpu_id)
{
#ifdef DEBUG_LEVEL_0
	printf("vcpu_id: %lld catch init swiotlb, start_ipa: %lx, end_ipa: %lx\n", vcpu_id, start_ipa, end_ipa);
#endif
}

__secure_text
void handle_write_ttbr1(struct titanium_vm *vm, unsigned long orig_ttbr1, uint32_t vcpu_id)
{
	int ret;
	struct titanium_vcpu *vcpu = vm->vcpus[vcpu_id];
	struct s1mmu *s1mmu = vcpu->s1mmu;
	unsigned long shadow_ttbr1;

#ifdef DEBUG_LEVEL_1
	printf("handle_write_ttbr1: vcpu_id: %u, orig_ttbr1: %lx\n", vcpu_id, orig_ttbr1);
#endif
	/*
	 * Secondary core reuses the same kernel spt with boot core.
	 */
	if (vcpu_id == vm->boot_core_id) {
		ret = create_new_spt(s1mmu, orig_ttbr1, TYPE_TTBR1, &shadow_ttbr1);
		if (ret < 0) {
			printf("%s: create new ttbr1 spt failed\n", __func__);
		}
		set_shadow_ttbr1(s1mmu, shadow_ttbr1);
	}
}

__secure_text
void handle_write_ttbr0(struct s1mmu *s1mmu, unsigned long orig_ttbr0, uint32_t vcpu_id)
{
	int ret;
	unsigned long shadow_ttbr0;

	ret = create_new_spt(s1mmu, orig_ttbr0, TYPE_TTBR0, &shadow_ttbr0);
	if (ret < 0) {
		printf("%s: create new ttbr0 spt failed\n", __func__);
	}
	set_shadow_ttbr0(s1mmu, shadow_ttbr0);
#ifdef DEBUG_LEVEL_1
	printf("handle_write_ttbr0: vcpu_id: %u, orig_ttbr0: %lx, shadow_ttbr0: %lx\n", vcpu_id, orig_ttbr0, shadow_ttbr0);
#endif
}

__secure_text
void handle_create_ttbr0(struct s1mmu *s1mmu, unsigned long orig_ttbr0, uint32_t vcpu_id)
{
	int ret;
	unsigned long shadow_ttbr0;

	ret = create_new_spt(s1mmu, orig_ttbr0, TYPE_TTBR0, &shadow_ttbr0);
	if (ret < 0) {
		printf("%s: create new ttbr0 spt failed\n", __func__);
	}
#ifdef DEBUG_LEVEL_1
	printf("handle_create_ttbr0: vcpu_id: %u, orig_ttbr0: %lx, shadow_ttbr0: %lx\n", vcpu_id, orig_ttbr0, shadow_ttbr0);
#endif
}

__secure_text
void handle_switch_ttbr0(struct titanium_vcpu *vcpu, unsigned long orig_ttbr0, uint32_t vcpu_id)
{
	struct s1mmu *s1mmu = vcpu->s1mmu;

	switch_spt(s1mmu, orig_ttbr0, TYPE_TTBR0);
	vcpu->use_shadow_ttbr0 = true;
	vcpu->user_task_pid = vcpu->next_task_pid;
}

__secure_text
void handle_destroy_ttbr0(struct s1mmu *s1mmu, unsigned long orig_ttbr0, uint32_t vcpu_id)
{
	int ret;

	ret = destroy_spt(s1mmu, orig_ttbr0, TYPE_TTBR0);
	if (ret < 0) {
		printf("%s: destroy ttbr0 spt failed, orig_ttbr0: %lx\n", __func__, orig_ttbr0);
	}
#ifdef DEBUG_LEVEL_1
	printf("handle_destroy_ttbr0: vcpu_id: %u, orig_ttbr0: %lx\n", vcpu_id, orig_ttbr0);
#endif
}

#ifdef CONFIG_SEL1_OPT
__secure_text
void sel1_handle_write_ptes(struct titanium_vm *kvm_vm, uint32_t vcpu_id)
{
	struct secure_shared_mem *shared_mem = get_shared_mem(kvm_vm, vcpu_id);

	// if (shared_mem->pte_entry_idx == 100) {
	//     printf("...\n");
	// }
	for (int i = 0; i < shared_mem->pte_entry_idx; ++i) {
		write_spt_pte_1(kvm_vm->s1mmu, shared_mem->entrys[i].fault_ipa,
						shared_mem->entrys[i].val, shared_mem->entrys[i].old_val, shared_mem->entrys[i].level);
	}
	shared_mem->pte_entry_idx = 0;
}
#endif

__secure_text
void handle_sched(struct titanium_vm *kvm_vm, uint32_t core_id, uint32_t vcpu_id,
                  struct sec_shm *shared_mem, uint64_t pid)
{
struct titanium_vcpu *vcpu = kvm_vm->vcpus[vcpu_id];
#ifdef DEBUG_LEVEL_1
	char *comm = (char *)&shared_mem->arg1;

	print_locked("[core %u] sched to %s\n", vcpu_id, comm);
#endif
	if (pid == vcpu->user_task_pid) {
		vcpu->use_shadow_ttbr0 = true;
	} else {
		vcpu->use_shadow_ttbr0 = false;
	}
	vcpu->next_task_pid = pid;
}

__secure_text
void sel1_handle_dispatcher(struct titanium_vm *kvm_vm, uint32_t core_id, uint32_t vcpu_id)
{
	struct sec_shm *shm = get_shared_mem(kvm_vm, vcpu_id);
	u_register_t arg1, arg2, arg3;
	struct titanium_vcpu *vcpu = kvm_vm->vcpus[vcpu_id];
	int exit_id;

	if (shm == NULL) {
		printf("%s: shared_mem is NULL\n", __func__);
		return;
	}
	arg1 = shm->arg1;
	arg2 = shm->arg2;
	arg3 = shm->arg3;
	exit_id = shm->exit_reason;

	switch (exit_id) {
	case SEL1_EXIT_WFI:
		sel1_handle_wfi();
	break;
	case SEL1_EXIT_WRITE_TTBR0:
		handle_write_ttbr0(vcpu->s1mmu, arg1, vcpu_id);
	break;
	case SEL1_EXIT_CREATE_TTBR0:
		handle_create_ttbr0(vcpu->s1mmu, arg1, vcpu_id);
	break;
	case SEL1_EXIT_DESITROY_TTBR0:
		handle_destroy_ttbr0(vcpu->s1mmu, arg1, vcpu_id);
	break;
	case SEL1_EXIT_SWITCH_TTBR0:
		handle_switch_ttbr0(vcpu, arg1, vcpu_id);
	break;
	case SEL1_EXIT_WRITE_TTBR1:
		handle_write_ttbr1(kvm_vm, arg1, vcpu_id);
	break;
	case SEL1_EXIT_WRITE_SCTLR:
		handle_write_sctlr(vcpu, arg1, vcpu_id);
	break;
	case SEL1_EXIT_WRITE_VBAR:
		handle_write_vbar(kvm_vm, arg1, vcpu_id);
	break;
	case SEL1_EXIT_WRITE_TCR:
		handle_write_tcr(arg1, vcpu_id);
	break;
	case SEL1_EXIT_WRITE_MAIR:
		handle_write_mair(arg1, vcpu_id);
	break;
	case SEL1_EXIT_READ_MIDR:
		handle_read_midr(kvm_vm, shm);
	break;
	case SEL1_EXIT_READ_MPIDR:
		handle_read_mpidr(vcpu, shm);
	break;
	case SEL1_EXIT_WRITE_PTE:
	case SEL1_EXIT_HANDLE_WRITE_PGTBL:
		/*
			* arg1 - fault_ipa
			* arg2 - val which guest-linux will write
			* arg3 - level
			*/
		(void)write_spt_pte(vcpu->s1mmu, arg1, arg2, arg3);
	break;
	#ifdef CONFIG_SEL1_OPT
	case SEL1_EXIT_WRITE_PTES:
		sel1_handle_write_ptes(kvm_vm, vcpu_id);
	break;
	#endif
	case SEL1_EXIT_INIT_SWIOTLB:
		handle_init_swiotlb(arg1, arg2, vcpu_id);
	break;
	case SEL1_EXIT_END_GIC_HANDLE:
		/* Do nothing */
	break;
	case SEL1_EXIT_ENABLE_DAIF:
	case SEL1_EXIT_RESTORE_DAIF:
		/* Do nothing */
	break;
	case SEL1_EXIT_SCHED:
		handle_sched(kvm_vm, core_id, vcpu_id, shm, arg3);
	break;
	case SEL1_EXIT_TEST:
		printf("catch test, arg1: %lx, arg2: %lx, arg3: %lx\n", arg1, arg2, arg3);
	break;
	default:
	break;
	}

	return;
}
