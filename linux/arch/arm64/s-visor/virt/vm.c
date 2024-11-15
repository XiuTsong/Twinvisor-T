/*
 * @Description: titanium vm.c
 * @Date: 2024-11-08 14:40:40
 */

#include <s-visor/s-visor.h>
#include <s-visor/virt/vcpu.h>
#include <s-visor/common/list.h>
#include <s-visor/virt/vm.h>
#include <s-visor/sched/smp.h>
#include <s-visor/mm/mm.h>

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

/**
 * Set up a initial register stages for guest VCPU
 */
void __secure_text init_vms(void)
{
    /* Init vm list which holds all VM in titanium */
    list_init(&titanium_vm_list);

    memset(&global_titanium_states, 0, sizeof(struct titanium_state) * SVISOR_PHYSICAL_CORE_NUM);
    // init_vgic();
    // print_lock_init();
}

void __secure_text kvm_shared_memory_register(void)
{
	if (!titanium_shared_pages) {
		titanium_shared_pages = (uint64_t *)pa2va(titanium_shared_pages_phys);
		// printf("%s: %d shared memory addr is %p.\n", __func__, __LINE__,
		//         titanium_shared_pages);
	} else {
		// printf("%s: %d shared memory already set up\n", __func__, __LINE__);
	}
}
