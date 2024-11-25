/*
 * @Description: titanium vm.c
 * @Date: 2024-11-08 14:40:40
 */
#include <asm/page-def.h>
#include <linux/errno.h>
#include <linux/string.h>

#include <s-visor/s-visor.h>
#include <s-visor/common/list.h>
#include <s-visor/common/macro.h>
#include <s-visor/sched/smp.h>
#include <s-visor/mm/mm.h>
#include <s-visor/mm/buddy_allocator.h>
#include <s-visor/mm/page_allocator.h>
#include <s-visor/mm/sec_mmu.h>
#include <s-visor/virt/vcpu.h>
#include <s-visor/virt/vm.h>
#include <s-visor/virt/kvm_host_common.h>
#include <s-visor/virt/kvm_relay.h>

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

/**
 * Set up a initial register stages for guest VCPU
 */
void __secure_text init_vms(void)
{
    /* Init vm list which holds all VM in titanium */
    list_init(&titanium_vm_list);

    memset(&global_titanium_states, 0, sizeof(struct titanium_state) * SVISOR_PHYSICAL_CORE_NUM);
    // init_vgic();
    print_lock_init();
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

/**
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