/*
 * @Description: titanium stage2_mmu.c
 * @Date: 2024-11-18 15:55:47
 */

#include <asm/page-def.h>

#include <s-visor/mm/stage2_mmu.h>
#include <s-visor/mm/mm.h>
#include <s-visor/mm/sec_fixmap.h>
#include <s-visor/sched/smp.h>

#ifdef SVISOR_DEBUG
#pragma GCC optimize("O0")
#endif

#define next_table_paddr(entry) ((entry).table.next_table_addr << PAGE_SHIFT)

/*
 * Translate a stage-2 page table from IPN to HPN
 * Return: target pte_t, { .pte = 0 } for not found
 * Support 3-level stage-2 PT with 4K granule
 */
pte_t translate_stage2_pt(paddr_t s2pt_phys, paddr_t ipn)
{
	pte_t ret = { .pte = 0 };
	ptp_t *l1_table = NULL, *l2_table = NULL, *l3_table = NULL;
	pte_t l1_entry, l2_entry;
	uint32_t l1_index, l2_index, l3_index;
	unsigned int core_id = get_core_id();

	if (s2pt_phys == 0) {
		return ret;
	}
	l1_table = set_sec_fixmap_l1(s2pt_phys, core_id);

	l1_index = GET_IPN_OFFSET_L1(ipn);
	l1_entry = l1_table->ent[l1_index];
	clear_sec_fixmap_l1(core_id);
	if ((l1_entry.pte & ARM64_MMU_PTE_INVALID_MASK) == 0 ||
			(l1_entry.pte & PTE_DESCRIPTOR_MASK) != PTE_DESCRIPTOR_TABLE) {
		return ret;
	}
	l2_table = set_sec_fixmap_l2(next_table_paddr(l1_entry), core_id);

	l2_index = GET_IPN_OFFSET_L2(ipn);
	if (!l2_table) return ret;
	l2_entry = l2_table->ent[l2_index];
	if ((l2_entry.pte & ARM64_MMU_PTE_INVALID_MASK) == 0 ||
			(l2_entry.pte & PTE_DESCRIPTOR_MASK) != PTE_DESCRIPTOR_TABLE) {
		return ret;
	}
	l3_table = set_sec_fixmap_l3(next_table_paddr(l2_entry), core_id);
	l3_index = GET_IPN_OFFSET_L3(ipn);
	if (!l3_table) {
		return ret;
	}
	ret = l3_table->ent[l3_index];
	clear_sec_fixmap_l3(core_id);

	return ret;
}
