/*
 * @Description: titanium mm.c
 * @Date: 2024-11-08 15:32:36
 */
#include <asm/page-def.h>
#include <linux/memory.h>

#include <s-visor/s-visor.h>
#include <s-visor/mm/buddy_allocator.h>
#include <s-visor/mm/page_allocator.h>
#include <s-visor/mm/mm.h>
#include <s-visor/mm/mmu.h>
#include <s-visor/mm/sec_fixmap.h>
#include <s-visor/mm/stage1_mmu.h>
#include <s-visor/virt/sec_defs.h>
#include <s-visor/virt/vcpu.h>

#ifdef SVISOR_DEBUG
#pragma GCC optimize("O0")
#endif

uint64_t __secure_data current_cpu_stack_sps[SVISOR_PHYSICAL_CORE_NUM] = {0};
unsigned long __secure_data linux_vp_offset = -1;

static void __secure_text init_percpu_stack(void)
{
	unsigned int i = 0U;

	for (; i < SVISOR_PHYSICAL_CORE_NUM; i++) {
		uint64_t stack_page = (uint64_t)bd_alloc(PAGE_SIZE, 12);
		current_cpu_stack_sps[i] = stack_page + PAGE_SIZE;
	}
}

static void __secure_text map_address_high(void)
{
	s1_ptp_t *s1ptp = (s1_ptp_t *)SECURE_PG_DIR_VIRT;

	map_vfn_to_pfn(s1ptp, VFN(HYP_VECTOR_HIGH), va2pfn(titanium_hyp_vector));
	map_vfn_to_pfn(s1ptp, VFN(SWITCH_GATE_HIGH), va2pfn(switch_gate));
}

void __secure_text mm_primary_init(void)
{
	bd_init();
	secure_page_alloc_init();
	init_percpu_stack();
	sec_fixmap_init();
	/*
	 * n-visor has prepared pgtable for s-visor. So we only
	 * need to map titanium_hyp_vector and switch_gate in high address here.
	 */
	map_address_high();
}

void __secure_text mm_secondary_init(void)
{
	/* MMU already activate, do nothing */
}
