/*
 * @Description: titanium stage2_mmu.c
 * @Date: 2024-11-18 15:55:47
 */

#include "linux/types.h"
#include <asm/page-def.h>

#include <s-visor/mm/stage2_mmu.h>

/*
 * Translate a stage-2 page table from IPN to HPN
 * Return: target pte_t, { .pte = 0 } for not found
 * FIXME: only support 3-level stage-2 PT with 4K granule
 */
pte_t translate_stage2_pt(ptp_t *s2ptp, paddr_t ipn) {
	pte_t ret = { .pte = 0 };
	ptp_t *l2_table = NULL, *l3_table = NULL;
	pte_t l1_entry, l2_entry;
	uint32_t l1_shift, l2_shift, l3_shift;
	uint32_t l1_index, l2_index, l3_index;

	l1_shift = (3 - 1) * PAGE_ORDER;
	l1_index = (ipn >> l1_shift) & ((1UL << PAGE_ORDER) - 1);

	if (!s2ptp) return ret;
	l1_entry = s2ptp->ent[l1_index];
	if ((l1_entry.pte & ARM64_MMU_PTE_INVALID_MASK) == 0 ||
			(l1_entry.pte & PTE_DESCRIPTOR_MASK) != PTE_DESCRIPTOR_TABLE) {
		return ret;
	}
	l2_table = (ptp_t *)(l1_entry.table.next_table_addr << PAGE_SHIFT);

	l2_shift = (3 - 2) * PAGE_ORDER;
	l2_index = (ipn >> l2_shift) & ((1UL << PAGE_ORDER) - 1);

	if (!l2_table) return ret;
	l2_entry = l2_table->ent[l2_index];
	if ((l2_entry.pte & ARM64_MMU_PTE_INVALID_MASK) == 0 ||
			(l2_entry.pte & PTE_DESCRIPTOR_MASK) != PTE_DESCRIPTOR_TABLE) {
		return ret;
	}
	l3_table = (ptp_t *)(l2_entry.table.next_table_addr << PAGE_SHIFT);

	l3_shift = (3 - 3) * PAGE_ORDER;
	l3_index = (ipn >> l3_shift) & ((1UL << PAGE_ORDER) - 1);

	if (!l3_table) return ret;
	return l3_table->ent[l3_index];
}
