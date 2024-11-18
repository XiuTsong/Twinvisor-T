/*
 * @Description: titanium sel1_mmu.c
 * @Date: 2024-11-18 15:44:03
 */

#include <linux/errno.h>
#include <asm/sysreg.h>

#include <s-visor/s-visor.h>
#include <s-visor/mm/sec_mmu.h>
#include <s-visor/mm/stage2_mmu.h>
#include <s-visor/mm/stage1_mmu.h>
#include <s-visor/mm/mm.h>
#include <s-visor/mm/page_allocator.h>
#include <s-visor/mm/buddy_allocator.h>

#ifdef SVISOR_DEBUG
#pragma GCC optimize("O0")
#endif

/*
 * This file is used for handle spt.
 * spt is short for shadow page table
 */

static int sync_spt_page(struct s1mmu *s1mmu, const s1_ptp_t *orig_pgtbl_ipa,
			 s1_ptp_t *shadow_pgtbl, int level);
static int destroy_spts(struct s1mmu *s1mmu, enum destroy_opt opt);
static int free_spt_pte(s1_pte_t *shadow_entry, int level,
			struct ttbr_info *ttbr_info);
static int s1mmu_map_vfn_to_pfn(s1_ptp_t *s1ptp, vaddr_t vfn, paddr_t pfn,
				s1_pte_t *ptep);

/* Translate a s-vm ipa to hva */
__secure_text
static unsigned long ipa2hva(unsigned long vttbr_value, unsigned long ipa)
{
	unsigned long hpa = 0, hva = 0;
	unsigned long offset = ipa & PAGE_MASK;
	unsigned long s2pt_mask = ~(~((1UL << 48) - 1) | PAGE_MASK);
	pte_t target_pte;
	ptp_t *s2ptp = (ptp_t *)(vttbr_value & s2pt_mask);

	target_pte = translate_stage2_pt(s2ptp, (ipa >> PAGE_SHIFT));
	if (target_pte.l3_page.is_valid) {
		hpa = (target_pte.l3_page.pfn << PAGE_SHIFT) | offset;
	} else {
		printf("%s: target_pte is invalid\n", __func__);
	}

	/* In el1-visor, hpa == hva */
	hva = pa2va(hpa);

	return hva;
}

__secure_text
static unsigned long fixup_ipn_to_hpn(unsigned long vttbr_value,
									  unsigned long ipn)
{
	unsigned long hpn = 0;
	pte_t target_pte;
	unsigned long s2pt_mask = ~(~((1UL << 48) - 1) | PAGE_MASK);
	ptp_t *s2ptp = (ptp_t *)(vttbr_value & s2pt_mask);

	/* We only translate ipn within [0x40000, 0x80000) or
     * ipn == 0x8010 or 0x8011 to hpn.
     * See decode_kvm_vm_exit() & map_gicv_dev_memory().
     */
	if (!(ipn >= 0x40000 && ipn < 0x80000) && (ipn != 0x8010) &&
	    (ipn != 0x8011)) {
		return ipn;
	}
	if (ipn == 0x8010 || ipn == 0x8011) {
		printf("gicv dev memory\n");
	}
	target_pte = translate_stage2_pt(s2ptp, ipn);
	if (target_pte.l3_page.is_valid) {
		hpn = target_pte.l3_page.pfn;
	} else {
		/*
		 * Instruction abort simulate. n-visor will fixup this
		 * FIXME: Hard code here
		 */
		write_sysreg(0x86000047, esr_el1);
		write_sysreg(ipn << PAGE_SHIFT, far_el1);
		asm volatile("smc #0x20");
		target_pte = translate_stage2_pt(s2ptp, ipn);
		if (target_pte.l3_page.is_valid) {
			hpn = target_pte.l3_page.pfn;
		} else {
			printf("n-visor failed to fixup stage-2 page fault\n");
			asm volatile("b .\n");
		}
	}

	return hpn;
}

__secure_text
static int set_spt_l3_entry(struct s1mmu *s1mmu, const s1_pte_t *orig_entry,
							unsigned long ipn, s1_pte_t **l3_entry)
{
	unsigned long hpn;

	/* set flag */
	(*l3_entry)->pte = orig_entry->pte;
	hpn = fixup_ipn_to_hpn(s1mmu->vttbr_el2, ipn);
	(*l3_entry)->l3_page.pfn = hpn;
	(*l3_entry)->l3_page.is_page = 1;
	if (hpn == ipn) {
		/* if hpn == ipn, it means that there is no ipn->hpn mapping
		 * in vttbr. We should set the valid bit to 0 to ensure that
		 * accessing to this address will lead to page fault,
		 * thus we can simulate the stage-2 page fault and switch to
		 * nvisor. We also set ignored bit to recognize this
		 */
		(*l3_entry)->l3_page.is_valid = 0;
		(*l3_entry)->l3_page.ignored = 1;
	}

	return 0;
}

/*
 * Copy the @original_entry->pte to @shadow_entry.
 * We only need to set next_table_addr to @shadow_next_ptp
 * The other attribute is the same.
 */
__secure_text
static inline void copy_and_set_pte(s1_pte_t **shadow_entry,
									const s1_pte_t *orig_entry,
									s1_ptp_t *shadow_next_ptp)
{
	(*shadow_entry)->pte = orig_entry->pte;
	(*shadow_entry)->table.next_table_addr =
		(va2pa(shadow_next_ptp)) >> PAGE_SHIFT;
}

__secure_text
static inline void set_current_ttbr_info(struct s1mmu *s1mmu,
										 struct ttbr_info *ttbr_info)
{
	s1mmu->current_ttbr_info = ttbr_info;
}

__secure_text
static int split_need_realloc(s1_pte_t *l2_entry, const s1_pte_t *orig_l2_entry)
{
	s1_ptp_t *l3_page;

	if (IS_PTE_INVALID(orig_l2_entry->pte) ||
	    IS_PTE_TABLE(orig_l2_entry->pte)) {
		printf("orig entry does not refer to l2_block\n");
		return -1;
	}
	if (IS_PTE_INVALID(l2_entry->pte) || !IS_PTE_TABLE(l2_entry->pte)) {
		return 1;
	}
	l3_page = (s1_ptp_t *)pfn2va(l2_entry->table.next_table_addr);
	if (!l3_page) {
		printf("shadow split l2_block next_addr is 0\n");
		return 1;
	}

	return 0;
}

/*
 * Split a l2-block entry into l3-page entries
 */
__secure_text
static int do_l2_block_split(struct s1mmu *s1mmu, s1_pte_t *l2_entry,
							 const s1_pte_t *orig_l2_entry)
{
	s1_ptp_t *l3_page = NULL;
	s1_pte_t *l3_entry = NULL;
	unsigned long l2_pfn;
	unsigned long ipn;
	int index, ret;

	ret = split_need_realloc(l2_entry, orig_l2_entry);
	if (ret < 0) {
		printf("check split need realloc failed\n");
		return ret;
	}
	if (ret > 0) {
		/* Need realloc */
		l3_page = secure_page_alloc();
		ret = ptp_info_create(s1mmu->current_ttbr_info,
				      (unsigned long)orig_l2_entry,
				      (unsigned long)l3_page);
		if (ret < 0) {
			printf("ptp info create failed\n");
			return ret;
		}
	} else {
		l3_page = (s1_ptp_t *)pfn2va(l2_entry->table.next_table_addr);
	}
	if (!l3_page) {
		printf("%s bd_alloc failed\n", __func__);
		return -1;
	}
	memset(l3_page, 0, PAGE_SIZE);

	l2_entry->pte = 0;
	l2_entry->table.is_valid = 1;
	l2_entry->table.is_table = 1;
	l2_entry->table.next_table_addr = va2pfn(l3_page);
	l2_pfn = orig_l2_entry->l2_block.pfn << PAGE_ORDER;
	for (index = 0; index < PTE_NUM; ++index) {
		l3_entry = &(l3_page->ent[index]);
		l3_entry->pte = orig_l2_entry->pte;
		ipn = l2_pfn + index;
		set_spt_l3_entry(s1mmu, orig_l2_entry, ipn, &l3_entry);
	}

	return 0;
}

/*
 * sync_spt_pte/page is used to synchronize original pte/page to corresponding
 * shadow page table.
 */
__secure_text
static int sync_spt_pte(struct s1mmu *s1mmu, const s1_pte_t *orig_entry,
						s1_pte_t *shadow_entry, int level)
{
	void *shadow_next_ptp = NULL;
	s1_ptp_t *next_orig_pgtbl_ipa = NULL;
	int ret = 0;

	if (IS_PTE_INVALID(orig_entry->pte)) {
		return 0;
	}
	if (!IS_PTE_TABLE(orig_entry->pte) || level == 3) {
		switch (level) {
		case 0:
			printf("error, level 0 huge page\n");
			return -1;
		case 2:
			do_l2_block_split(s1mmu, shadow_entry, orig_entry);
			break;
		case 3:
			shadow_entry->pte = orig_entry->pte;
			set_spt_l3_entry(s1mmu, orig_entry,
					 orig_entry->l3_page.pfn,
					 &shadow_entry);
			break;
		default:
			printf("l1 block unsupported\n");
			return -1;
		}
		return 0;
	}
	shadow_next_ptp = secure_page_alloc();
	memset(shadow_next_ptp, 0, PAGE_SIZE);
	copy_and_set_pte(&shadow_entry, orig_entry, shadow_next_ptp);

	next_orig_pgtbl_ipa =
		(s1_ptp_t *)(orig_entry->table.next_table_addr << PAGE_SHIFT);

	/* Need to walk next page recursively */
	ret = sync_spt_page(s1mmu, next_orig_pgtbl_ipa, shadow_next_ptp,
			    level + 1);
	if (ret < 0) {
		printf("sync next page wrong\n");
		return ret;
	}

	return 0;
}

/*
 * Synchronize the current page table page
 * and recurse to the next level page table
 * Must set the current_ttbr_info pointer before calling this function
 */
__secure_text
static int sync_spt_page(struct s1mmu *s1mmu, const s1_ptp_t *orig_pgtbl_ipa,
						 s1_ptp_t *shadow_pgtbl, int level)
{
	uint32_t index;
	s1_pte_t *orig_entry;
	s1_pte_t *shadow_entry;
	s1_ptp_t *orig_pgtbl;
	int ret = 0;

	if (!IS_VALID_LEVEL(level)) {
		printf("sync_spt_page, wrong level: %d\n", level);
		return -1;
	}
	if (shadow_pgtbl == NULL) {
		printf("shadow_pgtbl is NULL\n");
		return -1;
	}
	ret = ptp_info_create(s1mmu->current_ttbr_info,
			      (unsigned long)orig_pgtbl_ipa,
			      (unsigned long)shadow_pgtbl);
	if (ret < 0) {
		printf("ptp info create failed\n");
		return ret;
	}
	orig_pgtbl = (s1_ptp_t *)ipa2hva(s1mmu->vttbr_el2,
					 (unsigned long)orig_pgtbl_ipa);
	if (orig_pgtbl == NULL) {
		printf("ipa not mapped in stage-2 pgtbl\n");
		return -1;
	}

	/* Copy the whole page first */
	memcpy((void *)shadow_pgtbl, (void *)orig_pgtbl, PAGE_SIZE);
	for (index = 0; index < PTE_NUM; ++index) {
		orig_entry = &(orig_pgtbl->ent[index]);
		shadow_entry = &(shadow_pgtbl->ent[index]);
		sync_spt_pte(s1mmu, orig_entry, shadow_entry, level);
	}
	return ret;
}

__secure_text
int sync_shadow_pgtbl(struct s1mmu *s1mmu, s1_ptp_t *table_ipa,
					  void *shadow_table, int start_level)
{
	int ret;

	ret = sync_spt_page(s1mmu, table_ipa, shadow_table, start_level);
	if (ret < 0) {
		printf("sync pgtbl failed\n");
		return ret;
	}

	/* TODO: Scan pgtbl and mark read-only */

	return ret;
}

__secure_text
int map_shm_in_spt(struct s1mmu *s1mmu, s1_ptp_t *s1ptp)
{
	int ret = 0;
	uint32_t core_id;
	struct sec_shm **shms = NULL;

	shms = s1mmu->shms;
	if (shms == NULL) {
		printf("shm_base is NULL\n");
		return -1;
	}
	for (core_id = 0U; core_id < PHYSICAL_CORE_NUM; ++core_id) {
		ret = s1mmu_map_vfn_to_pfn(s1ptp, VFN(GUEST_SHM_ADDR(core_id)),
								   va2pfn((unsigned long)shms[core_id]),
								   NULL);
		if (ret != 0) {
			printf("map shared mem in spt failed at core: %u\n",
			       core_id);
			return ret;
		}
	}

	return ret;
}

__secure_text
static int map_gate_in_spt(struct s1mmu *s1mmu, s1_ptp_t *s1ptp,
						   enum ttbr_type type)
{
	int ret = 0;

	if (type == TYPE_TTBR0) {
		ret = s1mmu_map_vfn_to_pfn(s1ptp, VFN(va2pa(enter_guest)),
								   va2pfn(enter_guest), NULL);
		if (ret != 0) {
			goto out_map_gate;
		}
		ret = s1mmu_map_vfn_to_pfn(s1ptp,
								   VFN(va2pa(titanium_hyp_vector)),
								   va2pfn(titanium_hyp_vector),
								   NULL);
		if (ret != 0) {
			goto out_map_gate;
		}
	} else {
		/* FIXME: In optimization, we need to map at least 10 times pages */
		ret = map_shm_in_spt(s1mmu, s1ptp);
		if (ret != 0) {
			goto out_map_gate;
		}
	}

out_map_gate:
	return ret;
}

__secure_text
int create_new_spt(struct s1mmu *s1mmu, unsigned long orig_ttbr,
				   enum ttbr_type type, unsigned long *shadow_ttbr)
{
	void *new_pgd_page = NULL;
	struct ttbr_info *new_ttbr_info = NULL;
	int ret;

	new_pgd_page = secure_page_alloc();
	if (!new_pgd_page) {
		return 0;
	}
	memset(new_pgd_page, 0, PAGE_SIZE);

	/* There are only one ttbr1 spt.
     * So we should destroy the original ttbr1 spt first
     */
	if (type == TYPE_TTBR1) {
		ret = destroy_spts(s1mmu, DESTROY_TTBR1);
		if (ret < 0) {
			printf("destroy original ttbr1 spt failed\n");
			goto out_free_new_pgd;
		}
	}
	ret = ttbr_info_create(get_ttbr_info_list(s1mmu, type), orig_ttbr,
			       (unsigned long)new_pgd_page, type,
			       &new_ttbr_info);
	if (ret < 0) {
		printf("create ttbr_info failed\n");
		goto out_free_new_pgd;
	}
	// printf("ttbr_info_create success, new_ttbr_info: %p\n", new_ttbr_info);

	/* Set currrent_ttbr_info pointer before sync pgtbl */
	set_current_ttbr_info(s1mmu, new_ttbr_info);
	ret = sync_shadow_pgtbl(s1mmu, (s1_ptp_t *)orig_ttbr,
				(void *)new_pgd_page, 0);
	if (ret < 0) {
		printf("sync_pgtlb failed\n");
		goto out_remove_ttbr_info;
	}

	/* Map gate/hyp_vector in spt */
	ret = map_gate_in_spt(s1mmu, new_pgd_page, type);
	if (ret < 0) {
		printf("map gate in spt failed\n");
		goto out_remove_ttbr_info;
	}
	if (shadow_ttbr != NULL) {
		*shadow_ttbr = (unsigned long)new_pgd_page;
	}

	return 0;

out_remove_ttbr_info:
	ttbr_info_release(new_ttbr_info);
out_free_new_pgd:
	secure_page_free(new_pgd_page);

	return ret;
}

__secure_text
static int create_tmp_spt(struct s1mmu *s1mmu)
{
	s1_ptp_t *tmp_ttbr0 = 0;
	s1_ptp_t *tmp_ttbr1 = (s1_ptp_t *)get_tmp_ttbr1(s1mmu);

	tmp_ttbr0 = secure_page_alloc();
	if (tmp_ttbr0 == NULL) {
		goto out_bd_alloc_failed;
	}
	memset(tmp_ttbr0, 0, PAGE_SIZE);
	map_gate_in_spt(s1mmu, tmp_ttbr0, TYPE_TTBR0);
	set_tmp_ttbr0(s1mmu, (unsigned long)tmp_ttbr0);
	printf("create tmp ttbr0 %p\n", tmp_ttbr0);

	/* Only create tmp ttbr1 once */
	if (tmp_ttbr1 == 0) {
		tmp_ttbr1 = secure_page_alloc();
		if (tmp_ttbr1 == 0) {
			secure_page_free(tmp_ttbr0);
			goto out_bd_alloc_failed;
		}
		memset(tmp_ttbr1, 0, PAGE_SIZE);
		set_tmp_ttbr1(s1mmu, (unsigned long)tmp_ttbr1);
		printf("create tmp ttbr1 %p\n", tmp_ttbr1);
	}
	map_gate_in_spt(s1mmu, tmp_ttbr1, TYPE_TTBR1);

	return 0;
out_bd_alloc_failed:
	printf("%s: bd_alloc failed\n", __func__);
	return -1;
}

__secure_text
static int free_spt_page(s1_ptp_t *pgtbl, int level,
						 struct ttbr_info *ttbr_info)
{
	uint32_t index;
	s1_pte_t *entry;
	int ret = 0;

	if (!IS_VALID_LEVEL(level)) {
		printf("free_page, wrong level: %d\n", level);
		return -1;
	}
	if (pgtbl == NULL || level == 3) {
		return 0;
	}

	/* Walk pgtbl entry, we only need to set next_table_addr */
	for (index = 0; index < PTE_NUM; ++index) {
		entry = &(pgtbl->ent[index]);
		free_spt_pte(entry, level, ttbr_info);
	}

	return ret;
}

__secure_text
static void free_ptp(struct ttbr_info *ttbr_info, void *ptp)
{
	int ret = 0;

	if (ptp) {
		secure_page_free(ptp);
	}
	if (ttbr_info) {
		ret = ptp_info_release(ttbr_info, (unsigned long)ptp);
		if (ret < 0) {
			printf("%s: ptp_info remove failed\n", __func__);
		}
	}
}

__secure_text
static int free_spt_pte(s1_pte_t *entry, int level, struct ttbr_info *ttbr_info)
{
	void *next_ptp;
	int ret = 0;

	if (IS_PTE_INVALID(entry->pte) || !IS_PTE_TABLE(entry->pte) ||
	    level == 3) {
		entry->pte = 0;
		return 0;
	}
	next_ptp = (void *)pfn2va(entry->table.next_table_addr);
	if (next_ptp == NULL) {
		entry->pte = 0;
		return 0;
	}

	/* Need to walk next page recursively */
	ret = free_spt_page(next_ptp, level + 1, ttbr_info);
	if (ret < 0) {
		printf("free next page wrong\n");
		return ret;
	}
	free_ptp(ttbr_info, next_ptp);
	entry->pte = 0;

	return 0;
}

__secure_text
struct s1mmu *create_stage1_mmu(struct titanium_vm *vm, uint64_t core_id)
{
	struct s1mmu *s1mmu = NULL;
	int err;

	s1mmu = bd_alloc(sizeof(struct s1mmu), 0);
	memset(s1mmu, 0, sizeof(struct s1mmu));
	s1mmu->ttbr1_spt = &vm->ttbr1_spt;
	s1mmu->spt_info = &vm->spt_info;
	s1mmu->shms = vm->shms;
	err = create_tmp_spt(s1mmu);
	if (err != 0) {
		printf("create tmp spt failed\n");
		bd_free((void *)s1mmu);
		return NULL;
	}

	return s1mmu;
}

__secure_text
int destroy_tmp_spt(void *ttbr)
{
	int ret;
	s1_ptp_t *pgd = ttbr;

	ret = free_spt_page(pgd, 0, 0);
	if (ret < 0) {
		printf("destroy l1/l2/l3 page failed\n");
		return -1;
	}
	/* Free pgd */
	secure_page_free(pgd);

	return 0;
}

__secure_text
static int destroy_spt_with_ttbr_info(struct s1mmu *s1mmu,
									  struct ttbr_info *ttbr_info)
{
	s1_ptp_t *shadow_pgd;
	int ret = 0;

	shadow_pgd = (s1_ptp_t *)ttbr_info->shadow_ttbr;
	ret = free_spt_page(shadow_pgd, 0, ttbr_info);
	if (ret < 0) {
		printf("destroy l1/l2/l3 page failed\n");
	}

	/* TODO: Maybe we can use free_spt_pte directly? */
	free_ptp(ttbr_info, shadow_pgd);

	return ret;
}

__secure_text
static int destory_spts_with_ttbr_info_list(struct s1mmu *s1mmu,
											struct ttbr_info_list *ttbr_info_list)
{
	struct ttbr_info *ttbr_info_ptr, *tmp;
	int ret = 0;

	lock(&ttbr_info_list->lock);
	for_each_ttbr_info_safe(ttbr_info_ptr, tmp, ttbr_info_list)
	{
		ret = destroy_spt_with_ttbr_info(s1mmu, ttbr_info_ptr);
		if (ret < 0) {
			printf("destory ttbr0 spt with ttbr_info failed\n");
		}
		/* Free ttbr_info and remove from s1mmu list */
		ttbr_info_release(ttbr_info_ptr);
	}
	unlock(&ttbr_info_list->lock);

	return ret;
}

__secure_text
static int destroy_spts(struct s1mmu *s1mmu, enum destroy_opt opt)
{
	int ret = 0;

	if (opt != DESTROY_TTBR1) {
		ret = destory_spts_with_ttbr_info_list(
			s1mmu, get_ttbr_info_list(s1mmu, TYPE_TTBR0));
		if (ret < 0) {
			printf("destory ttbr0 spt with ttbr_info failed\n");
		}
	}
	if (opt != DESTROY_TTBR0) {
		ret = destory_spts_with_ttbr_info_list(
			s1mmu, get_ttbr_info_list(s1mmu, TYPE_TTBR1));
		if (ret < 0) {
			printf("destory ttbr1 spt with ttbr_info failed\n");
		}
	}

	return ret;
}

__secure_text
static int destroy_all_spts(struct s1mmu *s1mmu)
{
	return destroy_spts(s1mmu, DESTROY_ALL);
}

__secure_text
static int destroy_ttbr1_tmp_spt(struct s1mmu *s1mmu)
{
	int ret = 0;

	/* Only destory ttbr1 tmp spt once */
	if (get_tmp_ttbr1(s1mmu) != 0) {
		ret = destroy_tmp_spt((s1_ptp_t *)get_tmp_ttbr1(s1mmu));
		if (ret < 0) {
			return ret;
		}
		set_tmp_ttbr0(s1mmu, 0);
	}

	return ret;
}

__secure_text
int destroy_stage1_mmu(struct s1mmu *s1mmu, bool is_mmu_enable)
{
	int ret = 0;

	if (is_mmu_enable) {
		ret = destroy_all_spts(s1mmu);
		if (ret < 0) {
			goto out_destroy_spt_error;
		}
		printf("%s: destory all spt success\n", __func__);
	}
	ret = destroy_tmp_spt((s1_ptp_t *)get_tmp_ttbr0(s1mmu));
	if (ret < 0) {
		goto out_destroy_spt_error;
	}
	ret = destroy_ttbr1_tmp_spt(s1mmu);
	if (ret < 0) {
		goto out_destroy_spt_error;
	}
	bd_free(s1mmu);

	return ret;

out_destroy_spt_error:
	printf("%s: destory shadow pgtbl failed\n", __func__);
	return ret;
}

__secure_text
static int handle_write_orig_pte(unsigned long write_hva, unsigned long val,
								 int *need_free)
{
	s1_pte_t orig_pte;
	s1_pte_t new_pte;

	orig_pte.pte = *(unsigned long *)write_hva;
	if (IS_PTE_INVALID(orig_pte.pte) || !IS_PTE_TABLE(orig_pte.pte)) {
		*need_free = 0;
		goto out_write_pte;
	}

	/* orig_pte refers to a table */
	new_pte.pte = val;
	if (orig_pte.table.next_table_addr != new_pte.table.next_table_addr) {
		/*
		 * If next_table_addr is not the same, then we need to
		 * consider freeing the corresponding next shadow page
		 * before we use sync_page()
		 */
		*need_free = 1;
	}

out_write_pte:
	*(vaddr_t *)write_hva = val;
	return 0;
}

__secure_text
static int check_need_free(unsigned long orig_val, unsigned long val,
						   int *need_free)
{
	s1_pte_t orig_pte;
	s1_pte_t new_pte;

	orig_pte.pte = orig_val;
	if (IS_PTE_INVALID(orig_pte.pte) || !IS_PTE_TABLE(orig_pte.pte)) {
		*need_free = 0;
		return 0;
	}

	/* orig_pte refers to a table */
	new_pte.pte = val;
	if (orig_pte.table.next_table_addr != new_pte.table.next_table_addr) {
		*need_free = 1;
	}

	return 0;
}

/*
 * Get shadow_entry and store the result in @shadow_entry.
 * Original page table page and shadow page table page is 1:1 mapping.
 * All we need to do is to compute the offset.
 */
__secure_text
static s1_pte_t *get_shadow_entry(struct ptp_info *ptp_info,
								  unsigned long fault_ipa)
{
	unsigned long shadow_pgtbl_ipa;
	unsigned long shadow_pgtbl_hva;
	unsigned long offset;

	offset = fault_ipa - ptp_info->orig_ptp;
	shadow_pgtbl_ipa = ptp_info->shadow_ptp + offset;
	shadow_pgtbl_hva = pa2va(shadow_pgtbl_ipa);

	return (s1_pte_t *)shadow_pgtbl_hva;
}

/*
 * Write to vm's original page table and corresponding shadow page table.
 * Usually this happens due to guest-linux calling set_pgd/pud/pmd/pte.
 * There are special cases in which guest-linux do set pte_val
 * but does not call set_pgd/pud/pmd/pte. The solution now is to
 * proactively call_gate.
 * The relevant functions are recorded as followed(arch/arm64/include/asm/pgtable.h).
 * 1. __ptep_test_and_clear_young
 * 2. ptep_set_wrprotect
 * 3. ptep_get_and_clear
 */
__secure_text
int write_spt_pte(struct s1mmu *s1mmu, unsigned long fault_ipa,
		  unsigned long val, int level)
{
	struct ttbr_info *ttbr_info = NULL;
	struct ptp_info *ptp_info = NULL;
	vaddr_t fault_hva;
	s1_pte_t *orig_entry;
	s1_pte_t *shadow_entry;
	int need_free = 0;
	int ret;

	ret = spt_info_find_by_ipa(s1mmu->spt_info, fault_ipa, &ttbr_info,
				   &ptp_info);
	if (ret < 0) {
		fault_hva = ipa2hva(s1mmu->vttbr_el2, fault_ipa);
		printf("ttbr_info not found, ipa: %lx pa: %lx\n", fault_ipa,
		       fault_hva);
		return 0;
	}
	/* TODO: Security check here*/

	fault_hva = ipa2hva(s1mmu->vttbr_el2, fault_ipa);
	if (fault_hva == 0) {
		printf("ipa not mapped in stage-2 pgtbl\n");
		return -1;
	}

	/* Write original pgtbl pte first */
	ret = handle_write_orig_pte(fault_hva, val, &need_free);

	orig_entry = (s1_pte_t *)fault_hva;
	shadow_entry = get_shadow_entry(ptp_info, fault_ipa);

	/* Check whether we should free shadow pages */
	if (need_free == 1) {
		free_spt_pte(shadow_entry, level, ttbr_info);
	}

	set_current_ttbr_info(s1mmu, ttbr_info);
	ret = sync_spt_pte(s1mmu, orig_entry, shadow_entry, level);
	if (ret < 0) {
		printf("%s sync_spt_pte failed\n", __func__);
		return ret;
	}

	/* I feeeeel sorry, but I must check here whether the 'gate'
     * has been mapped, because write_spt_pte() may destroy the
     * originally mapping.
     * FIXME: Maybe we could find a better way to solve this problem.
     */
	map_gate_in_spt(s1mmu, (s1_ptp_t *)ttbr_info->shadow_ttbr,
			ttbr_info->type);
	// if (ttbr_info->type == TYPE_TTBR0) {
	//     printf("write_spt_pte, fault_ipa: %lx val: %lx, level: %d\n", fault_ipa, val, level);
	// }

	/* TODO: update read-only */
	return ret;
}

__secure_text
static unsigned long shadow_ttbr_with_asid(unsigned long original_ttbr_ipa,
										   unsigned long shadow_ttbr_ipa)
{
	unsigned long asid_field;
	asid_field = original_ttbr_ipa & (~BADDR_MASK);
	return (asid_field | shadow_ttbr_ipa);
}

__secure_text
void switch_spt(struct s1mmu *s1mmu, unsigned long ttbr, enum ttbr_type type)
{
	unsigned long shadow_ttbr;
	struct ttbr_info *ttbr_info;
	int ret;

	ret = spt_info_find_by_ttbr(s1mmu->spt_info, ttbr, type, &ttbr_info);
	if (ret < 0) {
		printf("No SPT found, ttbr val: %lx\n", ttbr);
		return;
	}
	shadow_ttbr = ttbr_info->shadow_ttbr;
	set_shadow_ttbr(s1mmu, shadow_ttbr_with_asid(ttbr, shadow_ttbr), type);
	set_current_ttbr_info(s1mmu, ttbr_info);
	// printf("switch spt to ttbr: %lx shadow_ttbr: %lx\n", ttbr, shadow_ttbr);
}

/*
 * Destory a spt which original pgbtl is @ttbr
 * Note that free_spt_page() here can only destory l1/l2/l3 page.
 * Hence we should later bd_free shadow_pgd and free corresponding
 * ptp_info and ttbr_info.
 */
__secure_text
int destroy_spt(struct s1mmu *s1mmu, unsigned long ttbr, enum ttbr_type type)
{
	s1_ptp_t *shadow_pgd;
	struct ttbr_info *ttbr_info;
	int ret = 0;

	ret = ttbr_info_list_erase(get_ttbr_info_list(s1mmu, type), ttbr,
				   &ttbr_info);
	if (ret < 0) {
		printf("ttbr_info_list erase failed , ttbr: %lx\n", ttbr);
	}
	shadow_pgd = (s1_ptp_t *)ttbr_info->shadow_ttbr;
	ret = free_spt_page(shadow_pgd, 0, ttbr_info);
	if (ret < 0) {
		printf("destroy l1/l2/l3 page failed\n");
	}

	free_ptp(ttbr_info, shadow_pgd);

	/* Free ttbr_info */
	bd_free((void *)ttbr_info);

	return ret;
}

/*
 * Translate VFN from a stage-1 PT
 * Return:
 * FIXME: only support 4-level stage-1 PT with 4K granule
 */
__secure_text
s1_pte_t translate_stage1_pt_s(s1_ptp_t *s1ptp, unsigned long vfn)
{
	s1_ptp_t *l0_table = NULL, *l1_table = NULL, *l2_table = NULL, *l3_table = NULL;
	s1_pte_t l0_entry, l1_entry, l2_entry, l3_entry = { .pte = 0 };
	uint32_t l0_shift, l1_shift, l2_shift, l3_shift;
	uint32_t l0_index, l1_index, l2_index, l3_index;

	l0_table = s1ptp;
	l0_shift = (3 - 0) * PAGE_ORDER;
	l0_index = (vfn >> l0_shift) & ((1UL << PAGE_ORDER) - 1);
	if (!l0_table)
		return l3_entry;
	l0_entry = l0_table->ent[l0_index];
	if (IS_PTE_INVALID(l0_entry.pte)) {
		return l3_entry;
	} else if (!IS_PTE_TABLE(l0_entry.pte)) {
		/* Huge page should be disabled */
		return l3_entry;
	}

	l1_table = (s1_ptp_t *)pfn2va(l0_entry.table.next_table_addr);
	l1_shift = (3 - 1) * PAGE_ORDER;
	l1_index = (vfn >> l1_shift) & ((1UL << PAGE_ORDER) - 1);
	if (!l1_table)
		return l3_entry;
	l1_entry = l1_table->ent[l1_index];
	if (IS_PTE_INVALID(l1_entry.pte)) {
		return l3_entry;
	} else if (!IS_PTE_TABLE(l1_entry.pte)) {
		/* Huge page should be disabled */
		return l3_entry;
	}

	l2_table = (s1_ptp_t *)pfn2va(l1_entry.table.next_table_addr);
	l2_shift = (3 - 2) * PAGE_ORDER;
	l2_index = (vfn >> l2_shift) & ((1UL << PAGE_ORDER) - 1);
	if (!l2_table)
		return l3_entry;
	l2_entry = l2_table->ent[l2_index];
	if (IS_PTE_INVALID(l2_entry.pte)) {
		return l3_entry;
	} else if (!IS_PTE_TABLE(l2_entry.pte)) {
		return l2_entry;
	}

	l3_table = (s1_ptp_t *)pfn2va(l2_entry.table.next_table_addr);
	l3_shift = (3 - 3) * PAGE_ORDER;
	l3_index = (vfn >> l3_shift) & ((1UL << PAGE_ORDER) - 1);
	if (!l3_table)
		return l3_entry;
	l3_entry = l3_table->ent[l3_index];

	return l3_entry;
}

__secure_text
static int s1mmu_map_vfn_to_pfn(s1_ptp_t *s1ptp, vaddr_t vfn, paddr_t pfn,
				s1_pte_t *ptep)
{
	s1_ptp_t *l0_table = NULL, *l1_table = NULL, *l2_table = NULL, *l3_table = NULL;
	s1_pte_t l0_entry, l1_entry, l2_entry, l3_entry;
	uint32_t l0_shift, l1_shift, l2_shift, l3_shift;
	uint32_t l0_index, l1_index, l2_index, l3_index;
	s1_ptp_t *next_ptp = NULL;

	l0_table = s1ptp;
	l0_shift = (3 - 0) * PAGE_ORDER;
	l0_index = (vfn >> l0_shift) & ((1UL << PAGE_ORDER) - 1);
	if (!l0_table) {
		return -EINVAL;
	}
	l0_entry = l0_table->ent[l0_index];
	if (IS_PTE_INVALID(l0_entry.pte)) {
		next_ptp = (s1_ptp_t *)secure_page_alloc();
		if (!next_ptp) {
			return -ENOMEM;
		}
		memset(next_ptp, 0, PAGE_SIZE);

		l0_entry.table.is_valid = 1;
		l0_entry.table.is_table = 1;
		l0_entry.table.NS = 0;
		l0_entry.table.next_table_addr = va2pfn(next_ptp);
		l0_table->ent[l0_index] = l0_entry;
	} else if (!IS_PTE_TABLE(l0_entry.pte)) {
		/* Huge page should be disabled */
		return -EINVAL;
	}

	l1_table = next_ptp;
	l1_shift = (3 - 1) * PAGE_ORDER;
	l1_index = (vfn >> l1_shift) & ((1UL << PAGE_ORDER) - 1);
	if (!l1_table)
		return -EINVAL;
	l1_entry = l1_table->ent[l1_index];
	if (IS_PTE_INVALID(l1_entry.pte)) {
		next_ptp = (s1_ptp_t *)secure_page_alloc();
		if (!next_ptp) {
			return -ENOMEM;
		}
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
	}

	l2_table = next_ptp;
	l2_shift = (3 - 2) * PAGE_ORDER;
	l2_index = (vfn >> l2_shift) & ((1UL << PAGE_ORDER) - 1);
	if (!l2_table)
		return -EINVAL;
	l2_entry = l2_table->ent[l2_index];
	if (IS_PTE_INVALID(l2_entry.pte)) {
		next_ptp = (s1_ptp_t *)secure_page_alloc();
		if (!next_ptp) {
			return -ENOMEM;
		}
		memset(next_ptp, 0, PAGE_SIZE);

		l2_entry.table.is_valid = 1;
		l2_entry.table.is_table = 1;
		l2_entry.table.NS = 0;
		/* FIXME: Should use virt_to_phys if MMU is enabled */
		l2_entry.table.next_table_addr = va2pfn(next_ptp);
		l2_table->ent[l2_index] = l2_entry;
	} else if (!IS_PTE_TABLE(l2_entry.pte)) {
		/* Huge page should be disabled */
		return -EINVAL;
	}

	l3_table = next_ptp;
	l3_shift = (3 - 3) * PAGE_ORDER;
	l3_index = (vfn >> l3_shift) & ((1UL << PAGE_ORDER) - 1);
	if (!l3_table)
		return -EINVAL;
	l3_entry = l3_table->ent[l3_index];
	if (!l3_entry.l3_page.is_valid) {
		if (pfn == 0) {
			/* New anonymous shadow DMA buffer! */
			pfn = ((paddr_t)secure_page_alloc() >> PAGE_SHIFT);
			if (!pfn)
				asm volatile("b .\n\t");
		}
		l3_entry.pte = 0;

		l3_entry.l3_page.is_valid = 1;
		l3_entry.l3_page.is_page = 1;
		l3_entry.l3_page.SH = 0x3;
		l3_entry.l3_page.AF = 1;
		l3_entry.l3_page.nG = 0;
		l3_entry.l3_page.pfn = pfn;
	}

	if (ptep)
		*ptep = l3_entry;
	l3_table->ent[l3_index] = l3_entry;

	return 0;
}

#ifdef CONFIG_SEL1_OPT
int write_spt_pte_1(struct s1mmu *s1mmu, unsigned long fault_ipa,
		    unsigned long val, unsigned long old_val, int level)
{
	struct ttbr_info *ttbr_info = NULL;
	struct ptp_info *ptp_info = NULL;
	addr_t fault_hva;
	s1_pte_t *old_entry;
	s1_pte_t *shadow_entry;
	int need_free = 0;
	int ret;

	ret = get_ttbr_info_with_ipa(s1mmu, fault_ipa, &ttbr_info, &ptp_info);
	if (ret == 0) {
		fault_hva = ipa2hva(s1mmu->vttbr_el2, fault_ipa);
		printf("ttbr_info not found, ipa: %lx pa: %lx\n", fault_ipa,
		       fault_hva);
		return -1;
	}

	fault_hva = ipa2hva(s1mmu->vttbr_el2, fault_ipa);
	if (fault_hva == 0) {
		printf("ipa not mapped in stage-2 pgtbl\n");
		return -1;
	}

	old_entry = (s1_pte_t *)fault_hva;
	shadow_entry = get_shadow_entry(ptp_info, fault_ipa);

	/* Check whether we should free shadow pages */
	if (need_free == 1) {
		free_spt_pte(shadow_entry, level, ttbr_info);
	}

	ret = sync_spt_pte(s1mmu, old_entry, shadow_entry, level);
	if (ret < 0) {
		printf("%s sync_spt_pte failed\n", __func__);
		return ret;
	}

	map_gate_in_spt(s1mmu, (s1_ptp_t *)ttbr_info->shadow_ttbr,
			ttbr_info->ttbr0_or_1);

	/* TODO: update read-only */

	return ttbr_info->ttbr0_or_1;
}
#endif
