/*
 * @Date: 2024-11-29 16:35:06
 */

#include <asm/page-def.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>
#include <linux/errno.h>

#include <s-visor/s-visor.h>
#include <s-visor/common/macro.h>
#include <s-visor/lib/stdint.h>
#include <s-visor/mm/sec_fixmap.h>
#include <s-visor/mm/mmu.h>
#include <s-visor/mm/mm.h>
#include <s-visor/mm/stage1_mmu.h>

#ifdef SVISOR_DEBUG
#pragma GCC optimize("O0")
#endif

#if CONFIG_PGTABLE_LEVELS == 4
s1_pte_t sec_fixmap_page_l1[PGTBL_4K_ENTRIES] __secure_data __page_aligned;
#endif
s1_pte_t sec_fixmap_page_l2[PGTBL_4K_ENTRIES] __secure_data __page_aligned;
s1_pte_t sec_fixmap_page_l3[PGTBL_4K_ENTRIES] __secure_data __page_aligned;

__secure_text
static int sec_fixmap_pgtable_init(s1_ptp_t *s1ptp, unsigned long sec_fixmap_base)
{
#if CONFIG_PGTABLE_LEVELS == 4
	s1_ptp_t *l0_table = NULL;
	s1_pte_t l0_entry;
	uint32_t l0_index;
#endif
	s1_ptp_t  *l1_table = NULL, *l2_table = NULL;
	s1_pte_t l1_entry, l2_entry;
	uint32_t l1_index, l2_index;
	s1_ptp_t *next_ptp = s1ptp;
	vaddr_t vfn = VFN(sec_fixmap_base);

#if CONFIG_PGTABLE_LEVELS == 4
	l0_table = s1ptp;
	l0_index = L0_INDEX(vfn);
	if (!l0_table) {
		return -EINVAL;
	}
	l0_entry = l0_table->ent[l0_index];
	if (IS_PTE_INVALID(l0_entry.pte)) {
		next_ptp = (s1_ptp_t *)sec_fixmap_page_l1;
		memset(next_ptp, 0, PAGE_SIZE);

		l0_entry.table.is_valid = 1;
		l0_entry.table.is_table = 1;
		l0_entry.table.NS = 0;
		l0_entry.table.next_table_addr = va2pfn(next_ptp);
		l0_table->ent[l0_index] = l0_entry;
	} else if (!IS_PTE_TABLE(l0_entry.pte)) {
		/* Huge page should be disabled */
		return -EINVAL;
	} else {
		next_ptp = (s1_ptp_t *)pfn2va(l0_entry.table.next_table_addr);
	}
#endif

	l1_table = next_ptp;
	l1_index = L1_INDEX(vfn);
	if (!l1_table)
		return -EINVAL;
	l1_entry = l1_table->ent[l1_index];
	if (IS_PTE_INVALID(l1_entry.pte)) {
		next_ptp = (s1_ptp_t *)sec_fixmap_page_l2;
		memset(next_ptp, 0, PAGE_SIZE);

		l1_entry.table.is_valid = 1;
		l1_entry.table.is_table = 1;
		l1_entry.table.NS = 0;
		/* Should use virt_to_phys if MMU is enabled */
		l1_entry.table.next_table_addr = va2pfn(next_ptp);
		l1_table->ent[l1_index] = l1_entry;
	} else if (!IS_PTE_TABLE(l1_entry.pte)) {
		/* Huge page should be disabled */
		return -EINVAL;
	} else {
		next_ptp = (s1_ptp_t *)pfn2va(l1_entry.table.next_table_addr);
	}

	l2_table = next_ptp;
	l2_index = L2_INDEX(vfn);
	if (!l2_table)
		return -EINVAL;
	l2_entry = l2_table->ent[l2_index];
	if (IS_PTE_INVALID(l2_entry.pte)) {
		next_ptp = (s1_ptp_t *)sec_fixmap_page_l3;
		if (!next_ptp) {
			return -ENOMEM;
		}
		memset(next_ptp, 0, PAGE_SIZE);

		l2_entry.table.is_valid = 1;
		l2_entry.table.is_table = 1;
		l2_entry.table.NS = 0;
		l2_entry.table.next_table_addr = va2pfn(next_ptp);
		l2_table->ent[l2_index] = l2_entry;
	} else if (!IS_PTE_TABLE(l2_entry.pte)) {
		/* Huge page should be disabled */
		return -EINVAL;
	} else {
		next_ptp = (s1_ptp_t *)pfn2va(l2_entry.table.next_table_addr);
	}

	return 0;
}

__secure_text
static inline s1_pte_t * sec_fixmap_pte(unsigned long vaddr)
{
	return (s1_pte_t *)&sec_fixmap_page_l3[L3_INDEX(VFN(vaddr))];
}

__secure_text
void __set_sec_fixmap(int idx, unsigned long phys, enum sec_fixmap_flag flag)
{
	unsigned long vaddr = __sec_fix_to_virt(idx);
	s1_pte_t *ptep = NULL;

	if (idx < 0 || idx >= MAX_SEC_FIXMAP_IDX) {
		printf("Invalid idx %d\n", idx);
		return;
	}

	ptep = sec_fixmap_pte(vaddr);
	if (flag == SEC_FIXMAP_FLAG_NORMAL) {
		ptep->l3_page.is_valid = 1;
		ptep->l3_page.is_page = 1;
		ptep->l3_page.SH = 0x3;
		ptep->l3_page.AF = 1;
		ptep->l3_page.nG = 0;
		ptep->l3_page.pfn = phys >> PAGE_SHIFT;
	} else {
		ptep->pte = 0;
		flush_tlb_kernel_range(vaddr, vaddr + PAGE_SIZE);
	}
}

__secure_text
void sec_fixmap_init(void)
{
	sec_fixmap_pgtable_init((s1_ptp_t *)SECURE_PG_DIR_VIRT, SEC_FIXMAP_BASE);
}

