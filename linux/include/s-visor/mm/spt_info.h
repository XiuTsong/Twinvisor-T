/*
 * @Description: titanium sel1_spt_info.h
 * @Date: 2024-11-18 14:25:42
 */

#ifndef __SVISOR_SPT_INFO_H__
#define __SVISOR_SPT_INFO_H__

#include <s-visor/common/list.h>
#include <s-visor/common/lock.h>

enum ttbr_type {
	TYPE_TTBR0,
	TYPE_TTBR1
};

struct ttbr_info {
	struct list_head node; /* For link into struct s1mmu */
	struct list_head ptp_info_list; /* For further checking */
	unsigned long orig_ttbr;
	unsigned long shadow_ttbr;
	enum ttbr_type type;
};

/* Page table pointer(ptp) info.
 * Given an orig ptp value, we can get correponding shadow ptp.
 */
struct ptp_info {
	struct list_head node;
	unsigned long orig_ptp;
	unsigned long shadow_ptp;
};

struct ttbr_info_list {
	struct lock lock;
	struct list_head head;
};

struct spt_info {
	struct ttbr_info_list ttbr0_info_list;
	struct ttbr_info_list ttbr1_info_list;
};

struct spt {
	/* TODO: Remove ttbr_info_list */
	struct list_head ttbr_info_list;
	unsigned long tmp_ttbr;
	unsigned long shadow_ttbr;
};

#define for_each_ttbr_info(ttbr_info_ptr, ttbr_info_list) \
		for_each_in_list(ttbr_info_ptr, struct ttbr_info, node, &ttbr_info_list->head)

#define for_each_ptp_info(ptp_info_ptr, ttbr_info) \
		for_each_in_list(ptp_info_ptr, struct ptp_info, node, &ttbr_info->ptp_info_list)

#define for_each_ttbr_info_safe(ttbr_info_ptr, tmp, ttbr_info_list) \
		for_each_in_list_safe(ttbr_info_ptr, tmp, struct ttbr_info, node, &ttbr_info_list->head)

int ptp_info_release(struct ttbr_info *ttbr_info, unsigned long shadow_pgtbl);
int ptp_info_create(struct ttbr_info *ttbr_info,
					unsigned long orig_pgtbl_ipa, unsigned long shadow_pgtbl);
int ptp_info_find(struct ttbr_info *ttbr_info, unsigned long ipa,
				  struct ptp_info **ptp_info);

int ttbr_info_create(struct ttbr_info_list *ttbr_info_list,
					 unsigned long orig_ttbr,
					 unsigned long shadow_ttbr,
					 enum ttbr_type type,
					 struct ttbr_info **new_ttbr_info);
void ttbr_info_release(struct ttbr_info *ttbr_info);

int ttbr_info_list_erase(struct ttbr_info_list *ttbr_info_list, unsigned long ttbr,
						 struct ttbr_info **ttbr_info);

void spt_info_init(struct spt_info *spt_info);
int spt_info_find_by_ipa(struct spt_info *spt_info, unsigned long ipa,
						 struct ttbr_info **ttbr_info, struct ptp_info **ptp_info);
int spt_info_find_by_ttbr(struct spt_info *spt_info, unsigned long ttbr,
						  enum ttbr_type type, struct ttbr_info **ttbr_info);


#endif
