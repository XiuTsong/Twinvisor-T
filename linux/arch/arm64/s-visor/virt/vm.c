/*
 * @Description: titanium vm.c
 * @Date: 2024-11-08 14:40:40
 */
#include "s-visor/mm/sec_shm.h"
#include "s-visor/virt/sec_defs.h"
#include <asm/page-def.h>
#include <asm/sysreg.h>
#include <asm/kvm_arm.h>
#include <asm/pgtable-hwdef.h>
#include <linux/errno.h>
#include <linux/string.h>

#include <s-visor/s-visor.h>
#include <s-visor/common/list.h>
#include <s-visor/common/macro.h>
#include <s-visor/lib/el3_runtime/smccc.h>
#include <s-visor/sched/smp.h>
#include <s-visor/mm/mm.h>
#include <s-visor/mm/mmu.h>
#include <s-visor/mm/buddy_allocator.h>
#include <s-visor/mm/page_allocator.h>
#include <s-visor/mm/sec_mmu.h>
#include <s-visor/mm/stage2_mmu.h>
#include <s-visor/virt/vcpu.h>
#include <s-visor/virt/vm.h>
#include <s-visor/virt/kvm_host_common.h>
#include <s-visor/virt/kvm_relay.h>
#include <s-visor/virt/vmexit_def.h>
#include <s-visor/virt/sec_vgic.h>
#include <s-visor/virt/sec_guest.h>

#ifdef SVISOR_DEBUG
#pragma GCC optimize("O0")
#endif

struct titanium_state __secure_data global_titanium_states[SVISOR_PHYSICAL_CORE_NUM];
uint64_t * __secure_data titanium_shared_pages = NULL;
uint64_t __secure_data titanium_shared_pages_phys;
unsigned long __secure_data g_tpidr[SVISOR_PHYSICAL_CORE_NUM];

struct list_head __secure_data titanium_vm_list;

struct titanium_state __secure_text *this_core_titanium_state(void)
{
	uint64_t core_id = get_core_id();

	return &global_titanium_states[core_id];
}

struct titanium_state __secure_text *get_titanium_state(uint64_t core_id)
{
	return &global_titanium_states[core_id];
}

static int __secure_text titanium_vm_enqueue(struct titanium_vm *vm)
{
	if (vm == NULL) {
		return -EINVAL;
	}

	list_push(&titanium_vm_list, &(vm->vm_list_node));
	return 0;
}

static int __secure_text titanium_vm_dequeue(struct titanium_vm *vm)
{
	if (vm == NULL) {
		return -EINVAL;
	}

	list_remove(&(vm->vm_list_node));
	return 0;
}

static int __secure_text alloc_shared_mem(struct titanium_vm *vm, uint64_t shared_mem_size)
{
	struct sec_shm* shared_mem = NULL;
	uint32_t core_id;

	for (core_id = 0; core_id < PHYSICAL_CORE_NUM; ++core_id) {
		if (GUEST_SHM_SIZE != PAGE_SIZE) {
			printf("warning, should adjust allocator here\n");
			asm volatile("b .\n");
		}
		shared_mem = (struct sec_shm *)secure_page_alloc();
		if (shared_mem == NULL) {
			printf("alloc shared memory failed\n");
			return -1;
		}
		memset((void *)shared_mem, 0, shared_mem_size);
		vm->shms[core_id] = shared_mem;
	}

	return 0;
}

void __secure_text kvm_shared_memory_register(void)
{
	if (!titanium_shared_pages) {
		titanium_shared_pages = (uint64_t *)pa2va(titanium_shared_pages_phys);
		printf("%s: %d shared memory addr is %p.\n", __func__, __LINE__,
		        titanium_shared_pages);
	} else {
		printf("%s: %d shared memory already set up\n", __func__, __LINE__);
	}
}

static void __secure_text destroy_kvm_vm(struct titanium_vm *vm, int vcpu_num)
{
	int i = 0;

	if (vm == NULL) {
		return;
	}
	for (i = 0; i < vcpu_num; i++) {
		bd_free(vm->vcpus[i]);
	}
	for (i = 0; i <PHYSICAL_CORE_NUM; i++) {
		secure_page_free(vm->shms[i]);
	}
}

/*
 * Create an empty vm struct for holding KVM controlling information
 */
static int __secure_text init_empty_vm(struct titanium_vm *vm, int vm_id, char *vm_name, int vcpu_num)
{
	int i = 0;
	int ret;

	if (vm == NULL) {
		return -EINVAL;
	}
	memset(vm, 0, sizeof(struct titanium_vm));
	vm->vm_id = vm_id;
	vm->vm_name = vm_name;
	vm->vm_state = VS_READY;
	vm->vm_type = VT_KVM; // this is a KVM VM
	vm->is_booted = false;
	list_init(&vm->vm_list_node);
	list_init(&vm->ttbr1_spt.ttbr_info_list);

	// create VCPUs for this VM
	for (i = 0; i < vcpu_num; i++) {
		struct titanium_vcpu *vcpu = (struct titanium_vcpu *)bd_alloc(sizeof(*vcpu), 0);
		memset(vcpu, 0, sizeof(struct titanium_vcpu));
		vcpu->vm = vm;
		vcpu->vcpu_id = i;
		vcpu->vcpu_state = VCPU_READY;
		vm->vcpus[i] = vcpu;
	}
	spt_info_init(&vm->spt_info);
	ret = alloc_shared_mem(vm, PAGE_SIZE);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void init_vgic(void)
{
	uint32_t i;

	for (i = 0U; i < PHYSICAL_CORE_NUM; ++i) {
		vgic_init(&global_titanium_states[i].vgic);
	}
}

/*
 * Set up a initial register stages for guest VCPU
 */
void __secure_text init_vms(void)
{
	/* Init vm list which holds all VM in titanium */
	list_init(&titanium_vm_list);

	memset(&global_titanium_states, 0, sizeof(struct titanium_state) * SVISOR_PHYSICAL_CORE_NUM);
	init_vgic();
	print_lock_init();
}


/*
 * Traverse titanium_vm_list and find target VM by @vm_id
 */
struct titanium_vm * __secure_text get_vm_by_id(int vm_id)
{
	struct titanium_vm *vm = NULL;
	for_each_in_list(vm, struct titanium_vm, vm_list_node, &titanium_vm_list) {
		if (vm->vm_id == vm_id)
			return vm;
	}
	return NULL; //not found
}

/* Sync 1 PTE from vttbr to shadow page table.
 * Only used in early stage.
 */
static void __secure_text sync_vttbr_to_spt(struct titanium_vm *target_vm, uint64_t vcpu_id)
{
	paddr_t fault_ipn = target_vm->vcpus[vcpu_id]->fault_ipn;
	paddr_t vttbr_value = target_vm->vttbr_el2;
	pte_t target_pte;
	struct titanium_vcpu *target_vcpu = target_vm->vcpus[vcpu_id];

	target_vcpu->is_s2pt_violation = 0;
	target_pte = translate_stage2_pt(vttbr_value & VTTBR_BADDR_MASK, fault_ipn);
	if (target_pte.l3_page.is_valid) {
		map_vfn_to_pfn_spt((s1_ptp_t *)get_tmp_ttbr0(target_vcpu->s1mmu), fault_ipn, target_pte.l3_page.pfn);
		// printf("sync fault_ipn: %lx to hpn: %llx\n", fault_ipn, target_pte.l3_page.pfn);
	} else {
		printf("%s: target_pte is invalid\n", __func__);
		asm volatile("b .\n");
	}
}

extern u_register_t set_guest_tpidr(u_register_t val);

static void __secure_text switch_tpidr(uint64_t core_id)
{
	u_register_t tpidr_el1_guest = g_tpidr[core_id];
	u_register_t tpidr_el1_svisor = read_sysreg(tpidr_el1);

	set_guest_tpidr(tpidr_el1_guest);
	g_tpidr[core_id] = tpidr_el1_svisor;
}

static inline void __secure_text prepare_shm_base(struct titanium_vm *vm)
{
	struct sec_shm *shm = get_shared_mem(vm, 0);

	/* Prefill pgdir phys in shm */
	shm->ttbr_phys = SECURE_PG_DIR_PHYS;
	/* Prefill high address offset in shm */
	shm->high_addr_offset = HYP_VECTOR_HIGH - (unsigned long)titanium_hyp_vector;
}

#define write_shared_mem(shm_base, pos, val)            \
	do {                                                \
		u_register_t *ptr = (u_register_t *)shm_base;   \
		*(ptr + pos) = val;                                     \
	} while (0)

/* Save guest's x0, x1, tpidr_el1, guest_vector in shared memory before enter_guest() */
static void __secure_text prepare_shared_memory(struct titanium_state *state, uint32_t core_id, uint32_t vcpu_id)
{
	struct gp_regs *gp_regs = &state->current_vcpu_ctx.gp_regs;
	struct sys_regs *sys_regs = &state->current_vcpu_ctx.sys_regs;
	struct titanium_vm *vm = state->current_vm;
	struct sec_shm *shm = NULL;

	shm = get_shared_mem(vm, core_id);
	if (shm == NULL) {
		printf("shared_mem is NULL\n");
		asm volatile("b .\n");
	}
	state->shared_mem = shm;
	/* TODO: Refactor struct secure_shared_mem */
	shm->x0 = gp_regs->x[0];
	shm->x1 = gp_regs->x[1];
	shm->tpidr = sys_regs->tpidr;
	shm->vbar_target = state->entry_helper.vbar_target;
	shm->guest_vector = vm->guest_vector;
	prepare_shm_base(vm);
}

static void __secure_text trace_vm_exit(struct titanium_state *state, int exit_code, uint32_t core_id)
{
	// uint64_t esr_el = get_guest_sys_reg(state, esr);
	// uint64_t fault_ipn = (get_host_reg(state, far_el1) >> 12);
	// uint64_t ec = ESR_EL_EC(esr_el);
	// uint64_t elr_el1 = get_guest_sys_reg(state, elr);

	// printf("[%d] core_id: %d ec: %llx, fault_ipn: %llx\n, elr_el1: 0x%llx\n", exit_code, core_id, ec, fault_ipn, elr_el1);
}

static void __secure_text prepare_jump_target(struct titanium_entry_helper *entry_helper,
											  uint64_t guest_vbar_base, int exit_code)
{
	entry_helper->jump_to_guest_vbar = true;
	entry_helper->vbar_target = guest_vbar_base;
	switch (exit_code) {
	case TITANIUM_VMEXIT_SYNC:
		entry_helper->vbar_target += 0x200;
	break;
	case TITANIUM_VMEXIT_IRQ:
		entry_helper->vbar_target += 0x280;
	break;
	case TITANIUM_VMEXIT_USER_SYNC:
		entry_helper->vbar_target += 0x400;
	break;
	case TITANIUM_VMEXIT_USER_IRQ:
		entry_helper->vbar_target += 0x480;
	break;
	default:
		printf("%s unsupported s_vm exit_code\n", __func__);
		asm volatile("b .\n");
	break;
	}
}

static int __secure_text fixup_titanium_vm_exit(struct titanium_state *state, int exit_code,
												uint32_t core_id, uint32_t vcpu_id)
{
	int ret = 0;
	struct titanium_entry_helper *entry_helper = &state->entry_helper;

	entry_helper->jump_to_guest_vbar = false;
	trace_vm_exit(state, exit_code, core_id);
#ifdef CONFIG_SEL1_OPT
	sel1_handle_write_ptes(state->current_vm, vcpu_id);
#endif
	if (exit_code == TITANIUM_VMEXIT_SYNC) {
		ret = decode_kvm_vm_exit(state, core_id, vcpu_id);
	}
	if ((exit_code == TITANIUM_VMEXIT_SYNC && ret == GUEST_SYNC) ||
		exit_code == TITANIUM_VMEXIT_IRQ ||
		exit_code == TITANIUM_VMEXIT_USER_SYNC ||
		exit_code == TITANIUM_VMEXIT_USER_IRQ) {
		ret = GUEST_SYNC;
		prepare_jump_target(entry_helper, state->current_vm->guest_vector, exit_code);
	}

	/* el0_fiq & el1h_fiq -> return 0, go to nvisor */
	return ret;
}

#define jump_to_el0_irq_guest(entry_helper, guest_vbar_base) \
    prepare_jump_target(entry_helper, guest_vbar_base, TITANIUM_VMEXIT_USER_IRQ)

#define jump_to_el1_irq_guest(entry_helper, guest_vbar_base) \
    prepare_jump_target(entry_helper, guest_vbar_base, TITANIUM_VMEXIT_IRQ)

static void __secure_text handle_vgic_interrupt(struct titanium_state *state, struct titanium_vm *target_vm,
												kvm_smc_req_t* kvm_smc_req, uint32_t core_id, uint32_t vcpu_id)
{
	struct sec_shm *shared_mem = get_shared_mem(target_vm, core_id);
	struct titanium_vgic *vgic = &state->vgic;
	int irqnr;

	vgic_add_lr_list(vgic, kvm_smc_req->icc_ich_lr);
	if (is_guest_local_irq_disable()) {
		return;
	}
	irqnr = vgic_inject_irq(vgic);
	if (irqnr >= 0) {
		/*
		 * Disable local irq may not be necessary
		 * as irq has already been masked in S-visor
		 */
		shared_mem->irqnr = irqnr;
		if (is_guest_user_mode()) {
			jump_to_el0_irq_guest(&state->entry_helper, target_vm->guest_vector);
		} else {
			jump_to_el1_irq_guest(&state->entry_helper, target_vm->guest_vector);
		}
	}
}

static inline void __secure_text init_vm_el2_regs(kvm_smc_req_t *kvm_smc_req, struct titanium_vm *vm)
{
	vm->vpidr_el2 = kvm_smc_req->el2_regs.vpidr_el2;
	vm->vttbr_el2 = kvm_smc_req->el2_regs.vttbr_el2;
}

static inline void __secure_text init_vcpu_el2_regs(kvm_smc_req_t *kvm_smc_req, struct titanium_vcpu *vcpu)
{
	vcpu->vmpidr_el2 = kvm_smc_req->el2_regs.vmpidr_el2;
}

static void __secure_text prepare_entry_helper(struct titanium_entry_helper *entry_helper,
											   struct titanium_vm *next_vm,
											   uint64_t vcpu_id)
{
	struct titanium_vcpu *vcpu = next_vm->vcpus[vcpu_id];
	struct s1mmu *s1mmu = vcpu->s1mmu;

	if (vcpu->is_vm_mmu_enable) {
		if (vcpu->use_shadow_ttbr0) {
			entry_helper->guest_ttbr0_el1 = get_shadow_ttbr0_phys(s1mmu);
		} else {
			entry_helper->guest_ttbr0_el1 = get_tmp_ttbr0_phys(s1mmu);
		}
		entry_helper->guest_ttbr1_el1 = get_shadow_ttbr1_phys(s1mmu);
	} else {
		entry_helper->guest_ttbr0_el1 = get_tmp_ttbr0_phys(s1mmu);
		entry_helper->guest_ttbr1_el1 = get_tmp_ttbr1_phys(s1mmu);
	}
}

static void __secure_text context_switch_to_vcpu(struct titanium_state *state,
												 struct titanium_vm *next_vm,
												 uint32_t vcpu_id, uint32_t core_id)
{
	if (next_vm == NULL) {
		_BUG("next_vm == NULL");
	}

	/* install next vm states */
	state->current_vm = next_vm;
	state->current_vcpu_id = vcpu_id;
	next_vm->vm_state = VS_RUNNING;
	next_vm->vcpus[vcpu_id]->vcpu_state = VCPU_RUNNING;

	// emulate_sys_reg_finish(state);
	prepare_entry_helper(&state->entry_helper, next_vm, vcpu_id);

	/* tpidr_el1 in titanium_state is svisor's
	 * we should switch it to guest's
	 * and fill g_tpidr with svisor's tpidr_el1
	 */
	switch_tpidr(core_id);

	/* This function should be call after switch_tpidr() */
	prepare_shared_memory(state, core_id, vcpu_id);

	if (!next_vm->is_booted) {
		next_vm->is_booted = true;
		next_vm->boot_core_id = vcpu_id;
	}
}

static void install_hyp_vector_high(void)
{
	write_sysreg((unsigned long)HYP_VECTOR_HIGH, vbar_el1);
}

static void restore_hyp_vector(void)
{
	write_sysreg((unsigned long)titanium_hyp_vector, vbar_el1);
}

/* forward smc request to the corresponding VM */
int __secure_text forward_smc_to_vm(struct titanium_state *state, int smc_type)
{
	struct titanium_vm *target_vm = NULL;
	uint64_t core_id = get_core_id();
	uint64_t vcpu_id = core_id;
	kvm_smc_req_t* kvm_smc_req = NULL;
	struct titanium_vcpu *target_vcpu = NULL;
	int exit_code;

	if (smc_type != ARM_SMCCC_KVM_TRAP_CALL) {
		_BUG("Unsupported smc type");
	}

	// TODO: support single kvm vm id is always 0
	kvm_smc_req = get_smc_req_region_sec(core_id);
	target_vm = get_vm_by_id(kvm_smc_req->sec_vm_id);
	if (target_vm == NULL) {
		printf("target vm %d is null", kvm_smc_req->sec_vm_id);
		_BUG("target vm NULL");
	}

	/* check whether KVM's updates are malicious */
	// check_vm_state(target_vm);

	if (!target_vm->is_booted) {
		init_vm_el2_regs(kvm_smc_req, target_vm);
	}

	target_vcpu = target_vm->vcpus[vcpu_id];
	init_vcpu_el2_regs(kvm_smc_req, target_vcpu);
	if (target_vcpu->is_s2pt_violation) {
		target_vcpu->s1mmu->vttbr_el2 = target_vm->vttbr_el2;
		sync_vttbr_to_spt(target_vm, vcpu_id);
	}

	do {
		/* If guest enable irq, then there may be interrupt to be injected. */
		handle_vgic_interrupt(state, target_vm, kvm_smc_req, core_id, vcpu_id);
		context_switch_to_vcpu(state, target_vm, vcpu_id, core_id);
		install_hyp_vector_high();
		exit_code = enter_guest(core_id);
		restore_hyp_vector();
	} while (fixup_titanium_vm_exit(state, exit_code, core_id, vcpu_id));

	state->current_vm->vm_state = VS_READY;
	state->current_vm->vcpus[vcpu_id]->vcpu_state = VCPU_READY;

	return exit_code;
}

void __secure_text kvm_shared_memory_handle(void)
{
	uint32_t core_id = get_core_id();
	kvm_smc_req_t* kvm_smc_req = get_smc_req_region_sec(core_id);
	struct titanium_vm *kvm_vm = NULL;
	uint32_t vcpu_num = PHYSICAL_CORE_NUM, i;

	switch (kvm_smc_req->req_type) {
		case REQ_KVM_TO_TITANIUM_FLUSH_IPA: {
			break;
		}
		case REQ_KVM_TO_TITANIUM_REMAP_IPA: {
			printf("REQ_KVM_TO_TITANIUM_REMAP_IPA unsupported now\n");
			break;
		}
		case REQ_KVM_TO_TITANIUM_UNMAP_IPA: {
			printf("REQ_KVM_TO_TITANIUM_UNMAP_IPA unsupported now\n");
			break;
		}
		case REQ_KVM_TO_TITANIUM_MEMCPY: {
			memcpy((void *)(kvm_smc_req->memcpy.dst_start_pfn << PAGE_SHIFT),
					(void *)(kvm_smc_req->memcpy.src_start_pfn << PAGE_SHIFT),
					kvm_smc_req->memcpy.nr_pages * PAGE_SIZE);
			break;
		}
		case REQ_KVM_TO_TITANIUM_BOOT: {
			printf("Boot kvm with id %d\n", kvm_smc_req->sec_vm_id);
			kvm_vm = (struct titanium_vm *)bd_alloc(sizeof(*kvm_vm), 0);
			printf("------------------------------\n");
			printf("Boot kvm %p with id %d\n", kvm_vm, kvm_smc_req->sec_vm_id);
			init_empty_vm(kvm_vm, kvm_smc_req->sec_vm_id, "kvm_vm", vcpu_num);
			for (i = 0U; i < vcpu_num; ++i) {
				/** For now, core_id == vcpu_id */
				core_id = i;
				kvm_vm->vcpus[i]->s1mmu = create_stage1_mmu(kvm_vm, core_id);
			}
			titanium_vm_enqueue(kvm_vm);
			printf("kvm %p created with id %d\n", kvm_vm, kvm_vm->vm_id);
			break;
		}
		case REQ_KVM_TO_TITANIUM_SHUTDOWN: {
			struct titanium_vm *kvm_vm = get_vm_by_id(kvm_smc_req->sec_vm_id);
			uint32_t vcpu_num = PHYSICAL_CORE_NUM, i;
			struct titanium_vcpu *vcpu = NULL;

			printf("Destroy kvm with id %d\n", kvm_smc_req->sec_vm_id);
			titanium_vm_dequeue(kvm_vm);
			printf("kvm %p dequeued with id %d\n", kvm_vm, kvm_vm->vm_id);
			for (i = 0U; i < vcpu_num; ++i) {
				vcpu = kvm_vm->vcpus[i];
				destroy_stage1_mmu(vcpu->s1mmu, vcpu->is_vm_mmu_enable);
			}
			// TODO: multi vcpu
			destroy_kvm_vm(kvm_vm, PHYSICAL_CORE_NUM);
			bd_free(kvm_vm);
			break;
		}
		default:
			_BUG("no such shared memory request\n");
			break;
	}
}
