/*
 * @Description: titanium sel1_spt_info.c
 * @Date: 2024-11-18 14:34:08
 */

#include <s-visor/s-visor.h>
#include <s-visor/mm/spt_info.h>
#include <s-visor/mm/buddy_allocator.h>
#include <s-visor/mm/stage1_mmu.h>

static inline bool __secure_text check_ipa_in_range(unsigned long pgtbl_ipa, unsigned long check_ipa)
{
	return (check_ipa >= pgtbl_ipa && check_ipa < (pgtbl_ipa + 0x1000));
}

/*
 * Free a struct ptp_info linked in given @ttbr_info->pgtbl_ipa_list_head
 * when ptp_info->shadow_pgtbl_ipa == @shadow_ipa
 * TODO: We need to opitimize this function later, maybe we should use a
 * tree to record ptp_info
 */
int __secure_text ptp_info_release(struct ttbr_info *ttbr_info, unsigned long shadow_pgtbl) {
	struct ptp_info *ptp_info_ptr = NULL;

	for_each_ptp_info(ptp_info_ptr, ttbr_info) {
		if (shadow_pgtbl == ptp_info_ptr->shadow_ptp) {
			list_remove(&(ptp_info_ptr->node));
			bd_free(ptp_info_ptr);
			return 0;
		}
	}
	// printf("ptp_info_release shadow_pgtbl: %lx not found\n", shadow_pgtbl);

	return 1;
}

/*
 * Record ptp_info when traverse the whole/partial pgtbl for future use.
 * e.g write_spt_pte will use ttbr_info list and corresponding ptp_info
 * list to match the given fault_ipa.
 * Maybe we could use better data structure or even better way to
 * record the information.
 */
int __secure_text ptp_info_create(struct ttbr_info *ttbr_info,
								  unsigned long orig_pgtbl_ipa,
								  unsigned long shadow_pgtbl)
{
	struct ptp_info *ptp_info = NULL;

	/* FIXME: Better way to record this node */
	ptp_info = bd_alloc(sizeof(struct ptp_info), 0);
	if (ptp_info == 0) {
		printf("bd_alloc ptp_info failed\n");
		return -1;
	}
	list_init(&(ptp_info->node));
	ptp_info->orig_ptp = orig_pgtbl_ipa;
	ptp_info->shadow_ptp = shadow_pgtbl;
	list_push(&ttbr_info->ptp_info_list, &ptp_info->node);
	// printf("--> ttbr_info: %p Add ptp_info shadow_ptp %lx\n", ttbr_info, shadow_pgtbl);

	return 0;
	}

int __secure_text ptp_info_find(struct ttbr_info *ttbr_info, unsigned long ipa,
								struct ptp_info **ptp_info)
{
	struct ptp_info *ptp_info_ptr = NULL;
	int ret = -1;

	for_each_ptp_info(ptp_info_ptr, ttbr_info) {
		if (check_ipa_in_range(ptp_info_ptr->orig_ptp, ipa)) {
			*ptp_info = ptp_info_ptr;
			ret = 0;
			break;
		}
	}

	return ret;
}

static inline int __secure_text is_ttbr_ipa_equal(unsigned long ttbr_a, unsigned long ttbr_b)
{
	unsigned long baddr_a = ttbr_a & (BADDR_MASK);
	unsigned long baddr_b = ttbr_b & (BADDR_MASK);

	return baddr_a == baddr_b;
}

static void __secure_text ttbr_info_init(struct ttbr_info *ttbr_info)
{
	list_init(&ttbr_info->node);
	list_init(&ttbr_info->ptp_info_list);
}

int __secure_text ttbr_info_create(struct ttbr_info_list *ttbr_info_list,
								   unsigned long orig_ttbr,
								   unsigned long shadow_ttbr,
								   enum ttbr_type type,
								   struct ttbr_info **ttbr_info)
{
	struct ttbr_info *new_ttbr_info = NULL;

	new_ttbr_info = bd_alloc(sizeof(struct ttbr_info), 0);
	if (new_ttbr_info){
		ttbr_info_init(new_ttbr_info);
		new_ttbr_info->orig_ttbr = orig_ttbr;
		new_ttbr_info->shadow_ttbr = shadow_ttbr;
		new_ttbr_info->type = type;
		lock(&ttbr_info_list->lock);
		list_push(&ttbr_info_list->head, &new_ttbr_info->node);
		unlock(&ttbr_info_list->lock);
		*ttbr_info = new_ttbr_info;
	} else {
		printf("bd_alloc ttbr_info failed\n");
		return -1;
	}

	return 0;
}

/*
 * Remove a ttbr_info from ttbr_info_list.
 * Call this function before ttbr_info_list lock is hold.
 */
void __secure_text ttbr_info_release(struct ttbr_info *ttbr_info)
{
	list_remove(&ttbr_info->node);
	bd_free((void *)ttbr_info);
}

static inline bool __secure_text check_ttbr_info_equal(struct ttbr_info *ttbr_info, unsigned long ttbr)
{
	/*
	 * FIXME: ttbr_ipa may be the original ttbr1_el1(the first time) or shadow_ttbr.
	 * Maybe we should use better way to distinguish
	 */
	return is_ttbr_ipa_equal(ttbr_info->orig_ttbr, ttbr) ||
			is_ttbr_ipa_equal(ttbr_info->shadow_ttbr, ttbr);
}

static void __secure_text ttbr_info_list_init(struct ttbr_info_list *ttbr_info_list)
{
	list_init(&ttbr_info_list->head);
	lock_init(&ttbr_info_list->lock);
}

static int __secure_text ttbr_info_list_find_lock(struct ttbr_info_list *ttbr_info_list, unsigned long ipa,
									struct ttbr_info **ttbr_info, struct ptp_info **ptp_info)
{
	struct ttbr_info *ttbr_info_ptr = NULL;
	struct ptp_info *ptp_info_ptr = NULL;
	int ret;

	lock(&ttbr_info_list->lock);
	for_each_ttbr_info(ttbr_info_ptr, ttbr_info_list) {
		ret = ptp_info_find(ttbr_info_ptr, ipa, &ptp_info_ptr);
		if (ret == 0)  {
			*ptp_info = ptp_info_ptr;
			*ttbr_info = ttbr_info_ptr;
			unlock(&ttbr_info_list->lock);
			return ret;
		}
	}
	unlock(&ttbr_info_list->lock);

	return -1;
}

int __secure_text ttbr_info_list_erase(struct ttbr_info_list *ttbr_info_list, unsigned long ttbr,
									   struct ttbr_info **ttbr_info)
{
	struct ttbr_info *ttbr_info_ptr = NULL;

	lock(&ttbr_info_list->lock);
	for_each_ttbr_info(ttbr_info_ptr, ttbr_info_list) {
		if (check_ttbr_info_equal(ttbr_info_ptr, ttbr)) {
			*ttbr_info = ttbr_info_ptr;
			list_remove(&ttbr_info_ptr->node);
			unlock(&ttbr_info_list->lock);
			return 0;
		}
	}
	unlock(&ttbr_info_list->lock);

	return -1;
}

void __secure_text spt_info_init(struct spt_info *spt_info)
{
	ttbr_info_list_init(&spt_info->ttbr0_info_list);
	ttbr_info_list_init(&spt_info->ttbr1_info_list);
}

int __secure_text spt_info_find_by_ipa(struct spt_info *spt_info, unsigned long ipa,
									   struct ttbr_info **ttbr_info, struct ptp_info **ptp_info)
{
	int ret;

	ret = ttbr_info_list_find_lock(&spt_info->ttbr1_info_list, ipa, ttbr_info, ptp_info);
	if (ret == 0) {
		return ret;
	}
	ret = ttbr_info_list_find_lock(&spt_info->ttbr0_info_list, ipa, ttbr_info, ptp_info);
	if (ret == 0) {
		return ret;
	}

	return -1;
}

int __secure_text spt_info_find_by_ttbr(struct spt_info *spt_info, unsigned long ttbr,
										enum ttbr_type type, struct ttbr_info **ttbr_info)
{
	struct ttbr_info *ttbr_info_ptr = NULL;
	struct ttbr_info_list *ttbr_info_list = NULL;

	if (type == TYPE_TTBR0) {
		ttbr_info_list = &spt_info->ttbr0_info_list;
	} else {
		ttbr_info_list = &spt_info->ttbr1_info_list;
	}
	lock(&ttbr_info_list->lock);
	for_each_ttbr_info(ttbr_info_ptr, ttbr_info_list) {
		if (check_ttbr_info_equal(ttbr_info_ptr, ttbr)) {
			*ttbr_info = ttbr_info_ptr;
			unlock(&ttbr_info_list->lock);
			return 0;
		}
	}
	unlock(&ttbr_info_list->lock);

	return -1;
}
