/*
 * @Date: 2024-11-29 16:22:17
 */

#ifndef __SVISOR_SEC_FIXMAP_H__
#define __SVISOR_SEC_FIXMAP_H__

#define SEC_FIXMAP_BASE 0xffffb00000000000
#define SEC_FIXMAP_SIZE SZ_2M

#define __sec_fix_to_virt(idx) (SEC_FIXMAP_BASE + idx * PAGE_SIZE)
#define __sec_virt_to_fix(virt) ((((virt) & PAGE_MASK) - SEC_FIXMAP_BASE) >> PAGE_SHIFT)

enum {
	/* L0~L3 stage-2 pgtable walk */
	SEC_FIXMAP_L0 = 0,
	SEC_FIXMAP_L1,
	SEC_FIXMAP_L2,
	SEC_FIXMAP_L3,
	/* PGD~PMD shadow pgtable walk */
	SEC_FIXMAP_PGD,
	SEC_FIXMAP_PUD,
	SEC_FIXMAP_PMD,
	SEC_FIXMAP_PTE,
	SEC_FIXMAP_LEVEL
};

enum sec_fixmap_flag {
	SEC_FIXMAP_FLAG_NORMAL,
	SEC_FIXMAP_FLAG_CLEAR
};

#define MAX_SEC_FIXMAP_IDX (SEC_FIXMAP_LEVEL * SVISOR_PHYSICAL_CORE_NUM)
#define SEC_FIXMAP_SPT_START SEC_FIXMAP_PGD

#define __set_sec_fixmap_offset(idx, phys, flags) \
({	\
	unsigned long ________addr;	\
	__set_sec_fixmap(idx, phys, flags);	\
	________addr = __sec_fix_to_virt(idx) + ((phys) & (PAGE_SIZE - 1));	\
	________addr;	\
})

#define set_sec_fixmap_offset(idx, phys) \
	__set_sec_fixmap_offset(idx, phys, SEC_FIXMAP_FLAG_NORMAL)

#define clear_sec_fixmap_offset(idx) \
	__set_sec_fixmap(idx, 0, SEC_FIXMAP_FLAG_CLEAR)

#define sec_fm_offset(core_id) ((core_id) * SEC_FIXMAP_LEVEL)

#define sec_fm_off_l0(core_id) (SEC_FIXMAP_L0 + sec_fm_offset(core_id))
#define sec_fm_off_l1(core_id) (SEC_FIXMAP_L1 + sec_fm_offset(core_id))
#define sec_fm_off_l2(core_id) (SEC_FIXMAP_L2 + sec_fm_offset(core_id))
#define sec_fm_off_l3(core_id) (SEC_FIXMAP_L3 + sec_fm_offset(core_id))

#define sec_fixmap_offset(core_id, level) (level + sec_fm_offset(core_id))

#define set_sec_fixmap_l0(paddr, core_id) \
	((void *)set_sec_fixmap_offset(sec_fm_off_l0(core_id), paddr))
#define set_sec_fixmap_l1(paddr, core_id) \
	((void *)set_sec_fixmap_offset(sec_fm_off_l1(core_id), paddr))
#define set_sec_fixmap_l2(paddr, core_id) \
	((void *)set_sec_fixmap_offset(sec_fm_off_l2(core_id), paddr))
#define set_sec_fixmap_l3(paddr, core_id) \
	((void *)set_sec_fixmap_offset(sec_fm_off_l3(core_id), paddr))

#define set_sec_fixmap(paddr, core_id, level) \
	((void *)set_sec_fixmap_offset(sec_fixmap_offset(core_id, level), paddr))

#define clear_sec_fixmap_l0(core_id) clear_sec_fixmap_offset(sec_fm_off_l0(core_id))
#define clear_sec_fixmap_l1(core_id) clear_sec_fixmap_offset(sec_fm_off_l1(core_id))
#define clear_sec_fixmap_l2(core_id) clear_sec_fixmap_offset(sec_fm_off_l2(core_id))
#define clear_sec_fixmap_l3(core_id) clear_sec_fixmap_offset(sec_fm_off_l3(core_id))

#define clear_sec_fixmap(core_id, level) \
	clear_sec_fixmap_offset(sec_fixmap_offset(core_id, level))

void sec_fixmap_init(void);
void __set_sec_fixmap(int idx, unsigned long phys, enum sec_fixmap_flag flag);

#endif
