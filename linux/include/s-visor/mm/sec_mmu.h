/*
 * @Description: titaninum sel1_mmu.h
 * @Date: 2024-11-18 15:50:09
 */

#ifndef __SVISOR_MM_SEC_MMU_H__
#define __SVISOR_MM_SEC_MMU_H__

#include <s-visor/mm/spt_info.h>
#include <s-visor/mm/stage1_mmu.h>
#include <s-visor/lib/stdint.h>
#include <s-visor/virt/vm.h>

struct s1mmu {
	struct ttbr_info *current_ttbr_info;
	struct spt *ttbr1_spt;
	struct spt ttbr0_spt;
	struct spt_info *spt_info;

	/* For ipa to hva use */
	unsigned long vttbr_el2;

	/* Point to struct titanium_vm->shms */
	struct sec_shm **shms;
};

enum destroy_opt {
	DESTROY_ALL,
	DESTROY_TTBR0,
	DESTROY_TTBR1
};

int create_new_spt(struct s1mmu *s1mmu, unsigned long old_ttbr, enum ttbr_type type, unsigned long *shadow_ttbr);
struct s1mmu* create_stage1_mmu(struct titanium_vm *vm, _uint64_t vcpu_id);
int destroy_stage1_mmu(struct s1mmu *s1mmu, _bool is_mmu_enable);
int destroy_tmp_spt(void *ttbr);
int write_spt_pte(struct s1mmu *s1mmu, paddr_t fault_ipa, paddr_t val, int level);
void switch_spt(struct s1mmu *s1mmu, unsigned long ttbr, enum ttbr_type type);
int destroy_spt(struct s1mmu *s1mmu, unsigned long ttbr, enum ttbr_type type);

/* 4-level stage-1 page table translation */
#define translate_spt translate_stage1_pt_s
s1_pte_t translate_stage1_pt_s(s1_ptp_t *s1ptp, vaddr_t vfn);

int map_shm_in_spt(struct s1mmu *s1mmu, s1_ptp_t *s1ptp);
int map_vfn_to_pfn_spt(s1_ptp_t *s1ptp, vaddr_t vfn, paddr_t pfn);

#ifdef CONFIG_SEL1_OPT
int write_spt_pte_1(struct s1mmu *s1mmu, paddr_t fault_ipa, paddr_t val, paddr_t old_val, int level);
#endif

static inline struct sec_shm* get_shared_mem(struct titanium_vm *vm, _uint32_t core_id)
{
	return vm->shms[core_id];
}

#define get_tmp_ttbr0(s1mmu) (s1mmu->ttbr0_spt.tmp_ttbr)
#define get_tmp_ttbr1(s1mmu) (s1mmu->ttbr1_spt->tmp_ttbr)
#define set_tmp_ttbr0(s1mmu, ttbr) (s1mmu->ttbr0_spt.tmp_ttbr = ttbr)
#define set_tmp_ttbr1(s1mmu, ttbr) (s1mmu->ttbr1_spt->tmp_ttbr = ttbr)
#define get_shadow_ttbr0(s1mmu) (s1mmu->ttbr0_spt.shadow_ttbr)
#define get_shadow_ttbr1(s1mmu) (s1mmu->ttbr1_spt->shadow_ttbr)
#define set_shadow_ttbr0(s1mmu, ttbr) (s1mmu->ttbr0_spt.shadow_ttbr = ttbr)
#define set_shadow_ttbr1(s1mmu, ttbr) (s1mmu->ttbr1_spt->shadow_ttbr = ttbr)

static inline void set_shadow_ttbr(struct s1mmu *s1mmu, unsigned long ttbr, enum ttbr_type type)
{
	if (type == TYPE_TTBR0) {
		set_shadow_ttbr0(s1mmu, ttbr);
	} else {
		set_shadow_ttbr1(s1mmu, ttbr);
	}
}

static inline struct ttbr_info_list* get_ttbr_info_list(struct s1mmu *s1mmu, enum ttbr_type type)
{
	if (type == TYPE_TTBR0) {
		return &s1mmu->spt_info->ttbr0_info_list;
	} else {
		return &s1mmu->spt_info->ttbr1_info_list;
	}
}


#endif
