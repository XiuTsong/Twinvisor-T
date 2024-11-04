extern bool sma_debug_flag;

static int __unmap_sma_page(struct page *page, int force,
        enum migrate_mode mode, enum migrate_reason reason)
{
	int rc = -EAGAIN;
    struct anon_vma *anon_vma = NULL;
	bool is_lru = !__PageMovable(page);

    if (!page->is_sec_mem) {
        pr_err("%s:%d non-SMA page %lx not supported!\n",
                __func__, __LINE__, page_to_pfn(page));
        asm volatile("b .\n\t");
    }

	if (!trylock_page(page)) {
		if (!force || mode == MIGRATE_ASYNC)
			goto out;

		/*
		 * It's not safe for direct compaction to call lock_page.
		 * For example, during page readahead pages are added locked
		 * to the LRU. Later, when the IO completes the pages are
		 * marked uptodate and unlocked. However, the queueing
		 * could be merging multiple pages for one bio (e.g.
		 * mpage_readpages). If an allocation happens for the
		 * second or third page, the process can end up locking
		 * the same page twice and deadlocking. Rather than
		 * trying to be clever about what pages can be locked,
		 * avoid the use of lock_page for direct compaction
		 * altogether.
		 */
		if (current->flags & PF_MEMALLOC)
			goto out;

		lock_page(page);
	}

	if (PageWriteback(page)) {
        pr_err("%s:%d disk page cache not support! pfn: %lx\n",
                __func__, __LINE__, page_to_pfn(page));
        asm volatile("b .\n\t");
	}

	/*
	 * By try_to_unmap(), page->mapcount goes down to 0 here. In this case,
	 * we cannot notice that anon_vma is freed while we migrates a page.
	 * This get_anon_vma() delays freeing anon_vma pointer until the end
	 * of migration. File cache pages are no problem because of page_lock()
	 * File Caches may use write_page() or lock_page() in migration, then,
	 * just care Anon page here.
	 *
	 * Only page_get_anon_vma() understands the subtleties of
	 * getting a hold on an anon_vma from outside one of its mms.
	 * But if we cannot get anon_vma, then we won't need it anyway,
	 * because that implies that the anon page is no longer mapped
	 * (and cannot be remapped so long as we hold the page lock).
	 */
	if (PageAnon(page) && !PageKsm(page))
		anon_vma = page_get_anon_vma(page);

	if (unlikely(!is_lru)) {
        pr_err("%s:%d ERROR: page %lx is_lru: %d\n",
                __func__, __LINE__, page_to_pfn(page), is_lru);
        asm volatile("b .\n\t");
	}

	if (!page->mapping) {
		VM_BUG_ON_PAGE(PageAnon(page), page);
		VM_BUG_ON_PAGE(page_has_private(page), page);
        if (!page_mapped(page)) {
            rc = MIGRATEPAGE_SUCCESS;
        }
        //pr_err("%s:%d ??? page %lx mapcount: %d, mapped: %x\n",
        //        __func__, __LINE__, page_to_pfn(page), 
        //        page_mapcount(page), page_mapped(page));
    } else if (page_mapped(page)) {
        bool is_unmapped = false;
		/* Establish migration ptes */
		VM_BUG_ON_PAGE(PageAnon(page) && !PageKsm(page) && !anon_vma,
				page);
		is_unmapped = try_to_unmap(page,
			TTU_MIGRATION|TTU_IGNORE_MLOCK|TTU_IGNORE_ACCESS);
        if (is_unmapped)
            rc = MIGRATEPAGE_SUCCESS;
        else {
            /* Failed to unmap old page, clear the swap_pte */
            remove_migration_ptes(page, page, false);
            unlock_page(page);
        }
    }

    /* Drop an anon_vma reference if we took one */
    if (anon_vma)
        put_anon_vma(anon_vma);
out:
	return rc;
}

static int unmap_sma_page(struct page *page, int force,
        enum migrate_mode mode, enum migrate_reason reason)
{
    return __unmap_sma_page(page, force, mode, reason);
}

static int unmap_sma_pages(struct list_head *from,
        enum migrate_mode mode, enum migrate_reason reason,
        struct list_head *unmapped_head)
{
	int retry = 1;
	int nr_succeeded = 0;
	int pass = 0;
	struct page *page;
	struct page *page2;
	
    int rc = MIGRATEPAGE_SUCCESS;

	for (pass = 0; pass < 10 && retry; pass++) {
		retry = 0;

		list_for_each_entry_safe(page, page2, from, lru) {
			cond_resched();

            rc = unmap_sma_page(page, pass > 2, mode, reason);
            if (page_count(page) != 1) {
                //if (sma_debug_flag)
                    pr_err("%s: WARNING pfn = %lx, count = %d, mapcount = %d\n",
                            __func__, page_to_pfn(page), page_count(page),
                            page_mapcount(page));
                set_page_count(page, 1);
            }

			switch(rc) {
			case -EAGAIN:
				retry++;
                if (pass == 10)
                    pr_err("%s: -EAGAIN pass = %d, retry = %d, pfn = %lx, mapcount = %d\n",
                            __func__, pass, retry, page_to_pfn(page),
                            page_mapcount(page));
				break;
			case MIGRATEPAGE_SUCCESS:
                if (page_count(page) != 1) {
                    //if (sma_debug_flag)
                        pr_err("%s: WARNING pfn = %lx, count = %d, mapcount = %d\n",
                                __func__, page_to_pfn(page), page_count(page),
                                page_mapcount(page));
                    set_page_count(page, 1);
                }
				nr_succeeded++;
                /* Move page from *from* to *unmapped_head* */
                list_del(&page->lru);
                list_add(&page->lru, unmapped_head);
				break;
			default:
                /* FIXME: how to handle unmapped pages */
                pr_err("%s:%d Invalid rc: %d, succeed: %d, retry: %d, page: %lx\n", 
                        __func__, __LINE__, rc, nr_succeeded, 
                        retry, page_to_pfn(page)); 
                asm volatile("b .\n\t");
				break;
			}
		}
	}
    
	return rc;
}

static int __move_sma_page(struct page *page, struct page *newpage,
        enum migrate_mode mode)
{
	int rc = -EAGAIN;
    struct anon_vma *anon_vma = NULL;
	
	/*
	 * Block others from accessing the new page when we get around to
	 * establishing additional references. We are usually the only one
	 * holding a reference to newpage at this point. We used to have a BUG
	 * here if trylock_page(newpage) fails, but would like to allow for
	 * cases where there might be a race with the previous use of newpage.
	 * This is much like races on refcount of oldpage: just don't BUG().
	 */
    if (unlikely(!trylock_page(newpage))) {
        pr_err("%s:%d ERROR: failed to lock newpage %lx\n",
                __func__, __LINE__, page_to_pfn(newpage));
        asm volatile("b .\n\t");
    }

	if (!page_mapped(page))
		rc = move_to_new_page(newpage, page, mode);
    else {
        pr_err("%s:%d ERROR: page %lx mapping: %px, mapcount: %d\n",
                __func__, __LINE__, page_to_pfn(page), page->mapping, 
                page_mapcount(page));
        asm volatile("b .\n\t");
    }

    /* page must be mapped in *__unmap_sma_page* after *try_to_unmap* */
#if 0
    remove_migration_ptes(page,
            rc == MIGRATEPAGE_SUCCESS ? newpage : page, false);
#else
    if (rc == MIGRATEPAGE_SUCCESS)
        remove_migration_ptes(page, newpage, false);
#endif
	
    unlock_page(newpage);
    if (rc != MIGRATEPAGE_SUCCESS)
        goto failed;
	
    /* Drop an anon_vma reference if we took one */
    anon_vma = page_get_anon_vma(page);
    if (anon_vma) {
        /* We get this anon_vma in *__unmap_sma_page* & previous line */
		put_anon_vma(anon_vma);
    }
    unlock_page(page);
failed:

    return rc;
}

static int move_sma_page(new_page_t get_new_page,
        free_page_t put_new_page, unsigned long private, 
        struct page *page, enum migrate_mode mode,
        enum migrate_reason reason)
{
	int rc = MIGRATEPAGE_SUCCESS;
    struct page *newpage;

    newpage = get_new_page(page, private);
    if (!newpage) {
        pr_err("%s:%d ERROR no newpage, old page refcount: %d\n",
                __func__, __LINE__, page_count(page));
        asm volatile("b .");
        return -ENOMEM;
    }
    if (page_count(newpage) != 1) {
        pr_err("%s:%d ERROR newpage refcount: %d\n",
                __func__, __LINE__, page_count(newpage));
    }
    
    rc = __move_sma_page(page, newpage, mode); 
	if (rc == MIGRATEPAGE_SUCCESS)
		set_page_owner_migrate_reason(newpage, reason);

    /*
	 * If migration is successful, releases reference grabbed during
	 * isolation. Otherwise, restore the page to right list unless
	 * we want to retry.
	 */
	if (rc == MIGRATEPAGE_SUCCESS) {
        /* 
         * SMA will handle this old page, do NOT put_page here.
         * *reason* must be MR_MEMORY_COMPACTION.
         */
        if (!newpage->is_sec_mem) {
            pr_err("%s:%d migrate to *non-secure* page! pfn %lx, count: %d:%d\n",
                    __func__, __LINE__, page_to_pfn(page),
                    page_count(page), page_mapcount(page));
            BUG();
        } else {
            if (page_count(page) != 1) {
                pr_err("%s:%d ERROR src pfn %lx, refcount: %d, mapcount: %d\n",
                        __func__, __LINE__, page_to_pfn(page),
                        page_count(page), page_mapcount(page));
            }
#if 0
            page->is_sec_mem = false;
#endif
        }
	} else if (rc == -EAGAIN) {
		if (put_new_page)
			put_new_page(newpage, private);
		else
			put_page(newpage);
    } else {
        pr_err("%s:%d ERROR: invalid rc: %d, PFN %lx, count: %d\n",
                __func__, __LINE__, rc, page_to_pfn(page), page_count(page));
        asm volatile("b .\n\t");
	}
	
    return rc;
}

static int move_sma_pages(new_page_t get_new_page,
        free_page_t put_new_page, unsigned long private, 
        struct list_head *unmapped_head, enum migrate_mode mode,
        enum migrate_reason reason, struct list_head *moved_head)
{
	int retry = 1;
	int nr_succeeded = 0;
	int pass = 0;
	struct page *page;
	struct page *page2;
	
    int rc = MIGRATEPAGE_SUCCESS;

	for (pass = 0; pass < 10 && retry; pass++) {
		retry = 0;

		list_for_each_entry_safe(page, page2, unmapped_head, lru) {
			cond_resched();

            /* FIXME: why an unmapped page has refcount > 1? */
            if (page_count(page) != 1) {
                //if (sma_debug_flag)
                    pr_err("%s:%d WARNING pfn = %lx, count = %d:%d, mapcount = %d\n",
                            __func__, __LINE__, page_to_pfn(page), 
                            page_count(page), page_ref_count(page),
                            page_mapcount(page));
                set_page_count(page, 1);
            }
            rc = move_sma_page(get_new_page, put_new_page, private, 
                    page, mode, reason);

			switch(rc) {
			case -EAGAIN:
				retry++;
                //if (sma_debug_flag)
                if (pass == 10)
                    pr_err("%s: -EAGAIN pass = %d, retry = %d, pfn = %lx,"
                            " count = %d, mapcount = %d\n",
                            __func__, pass, retry, page_to_pfn(page),
                            page_count(page), page_mapcount(page));
				break;
			case MIGRATEPAGE_SUCCESS:
				nr_succeeded++;
                /* Move page from *unmapped_head* to *moved_head* */
                list_del(&page->lru);
                list_add(&page->lru, moved_head);
                if (page_mapcount(page) != 0) {
                    pr_err("%s: NON-ZERO SRC pass = %d, retry = %d, pfn = %lx,"
                            " count = %d, mapcount = %d\n",
                            __func__, pass, retry, page_to_pfn(page),
                            page_count(page), page_mapcount(page));
                    page_mapcount_reset(page);
                }
#if 0
                if (page->is_sec_mem)
                    BUG();
#endif
				break;
			default:
                /* FIXME: how to handle moved pages */
                pr_err("%s:%d Invalid rc: %d, succeed: %d, retry: %d, page: %lx\n", 
                        __func__, __LINE__, rc, nr_succeeded, 
                        retry, page_to_pfn(page)); 
                asm volatile("b .\n\t");
				break;
			}
		}
	}
    
	return rc;
}

#include <linux/kvm_host.h>
#include <asm/kvm_host.h>
#include <linux/sma.h>
#include <linux/sort.h>

bool is_migrating = false;
uint32_t migrate_sec_vm_id = 0;
uint32_t nr_migrate_pages = 0;
/* 8M == 2048 pages */
uint64_t migrate_ipns[2048] = {0};

static int compare_64bit(const void *lhs, const void *rhs) {
    uint64_t l = *(const uint64_t *)(lhs);
    uint64_t r = *(const uint64_t *)(rhs);

    if (l < r) return -1;
    if (l > r) return 1;
    return 0;
}

static void set_ipn_ranges(kvm_smc_req_t *smc_req) {
    int i, j = 0;
    uint64_t prev_ipn;

    if (!nr_migrate_pages) {
        pr_err("%s: ERROR no IPNs available\n", __func__);
        asm volatile("b .\n\t");
    }
    
    sort(migrate_ipns, nr_migrate_pages, sizeof(uint64_t), compare_64bit, NULL);

    smc_req->unmap_ipa.ipn_ranges[j] = migrate_ipns[0];
    prev_ipn = migrate_ipns[0];
    for (i = 1; i < nr_migrate_pages; i++) {
        uint64_t ipn = migrate_ipns[i];
        if (ipn == prev_ipn + 1) {
            prev_ipn = ipn;
        } else {
            /* Previous range recorded in [..., prev_ipn] */
            smc_req->unmap_ipa.ipn_ranges[j + 1] = prev_ipn;
            if (sma_debug_flag)
                pr_err("%s: \t Insert range [%llx, %llx]\n", __func__,
                        smc_req->unmap_ipa.ipn_ranges[j],
                        smc_req->unmap_ipa.ipn_ranges[j + 1]);
            j += 2;
            
            /* Next range starts from [ipn, ...] */
            smc_req->unmap_ipa.ipn_ranges[j] = ipn;
            prev_ipn = ipn;
        }
    }
    /* Last range recorded in [..., prev_ipn] */
    smc_req->unmap_ipa.ipn_ranges[j + 1] = prev_ipn;
    if (sma_debug_flag)
        pr_err("%s: >>> Last range [%llx, %llx]\n", __func__,
                smc_req->unmap_ipa.ipn_ranges[j],
                smc_req->unmap_ipa.ipn_ranges[j + 1]);
    j += 2;
    /* End of range */
    smc_req->unmap_ipa.ipn_ranges[j] = 0;
}

/* 
 * Batching version of *migrate_pages* for SMA, *from* is sorted by PFN.
 * Only support CMA 4K pages now.
 */
int migrate_sma_pages(struct list_head *from, new_page_t get_new_page,
		free_page_t put_new_page, unsigned long private,
		enum migrate_mode mode, enum migrate_reason reason)
{
	int swapwrite = current->flags & PF_SWAPWRITE;
	int rc;
    struct list_head unmapped_head, moved_head;
    struct page *head_page = list_first_entry(from, struct page, lru); 
    unsigned long src_base_pfn = page_to_pfn(head_page);
    struct sec_mem_cache *dst_cache = (struct sec_mem_cache *)private;
    unsigned long dst_base_pfn = dst_cache->base_pfn;
    INIT_LIST_HEAD(&unmapped_head);
    INIT_LIST_HEAD(&moved_head);

	if (!swapwrite)
		current->flags |= PF_SWAPWRITE;

    /* Init variables to record IPA range, used in *handle_hva_to_gpa* */
    migrate_sec_vm_id = 0;
    nr_migrate_pages = 0;
    is_migrating = true;

    rc = unmap_sma_pages(from, mode, reason, &unmapped_head);
    if (rc != 0) {
        pr_err("%s:%d failed to unmap %d sma pages\n",
                __func__, __LINE__, rc);
        goto out;
    }
    is_migrating = false;

//    /* TODO: Notify titanium to unmap stage2 PT & copy secure memory */
//#if 0
//    // flush every page
//    flush_titanium_shadow_page_tables(); 
//#else
//    {
//        kvm_smc_req_t* kvm_smc_unmap_req = get_smc_req_region(smp_processor_id());
//        kvm_smc_unmap_req->sec_vm_id = migrate_sec_vm_id;
//        kvm_smc_unmap_req->req_type = REQ_KVM_TO_TITANIUM_UNMAP_IPA;
//        set_ipn_ranges(kvm_smc_unmap_req);
//        local_irq_disable();
//        asm volatile("smc #0x18\n");
//        local_irq_enable();
//    }
//#endif
//    /* 
//     * Notify titanium to copy secure memory.
//     * TODO: *nr_pages* can be less than (8M / 4K) if cache is not full
//     */
//    if (src_base_pfn & 0x7ff) {
//        pr_err("%s:%d base_pfn %lx is NOT 8M-aligned\n",
//                __func__, __LINE__, src_base_pfn);
//        asm volatile("b .\n\t");
//    }
//
//    /* Titanium has copied secure memory, change mode to SYNC_NO_COPY */
//#if 0
//    mode = MIGRATE_SYNC;
//#else
//    kvm_smc_req_t* smc_req = get_smc_req_region(smp_processor_id());
//    smc_req->req_type = REQ_KVM_TO_TITANIUM_MEMCPY;
//    smc_req->sec_vm_id = migrate_sec_vm_id;
//    smc_req->memcpy.src_start_pfn = src_base_pfn;
//    smc_req->memcpy.dst_start_pfn = dst_base_pfn;
//    smc_req->memcpy.nr_pages = (8 << (20 - 12));
//    local_irq_disable();
//    asm volatile("smc 0x18\n\t");
//    local_irq_enable();
//    /* Titanium has copied secure memory, change mode to SYNC_NO_COPY */
//    mode = MIGRATE_SYNC_NO_COPY;
//#endif

    if (src_base_pfn & 0x7ff) {
        pr_err("%s:%d base_pfn %lx is NOT 8M-aligned\n",
                __func__, __LINE__, src_base_pfn);
        asm volatile("b .\n\t");
    }
    printk("move from src 0x%llx to dst 0x%llx \n", src_base_pfn, dst_base_pfn);
    /* TODO: Notify titanium to unmap stage2 PT & copy secure memory */
    kvm_smc_req_t* smc_req = get_smc_req_region(smp_processor_id());
    smc_req->sec_vm_id = migrate_sec_vm_id;
    smc_req->req_type = REQ_KVM_TO_TITANIUM_REMAP_IPA;
    smc_req->remap_ipa.src_start_pfn = src_base_pfn;
    smc_req->remap_ipa.dst_start_pfn = dst_base_pfn;
    smc_req->remap_ipa.nr_pages = (8 << (20 - 12));
    memcpy(smc_req->remap_ipa.ipn_list, migrate_ipns, sizeof(migrate_ipns));
    local_irq_disable();
    asm volatile("smc #0x18\n");
    local_irq_enable();

    /* Titanium has copied secure memory, change mode to SYNC_NO_COPY */
    mode = MIGRATE_SYNC_NO_COPY;
    rc = move_sma_pages(get_new_page, put_new_page,
            private, &unmapped_head, mode, reason, &moved_head);
    if (rc != 0) {
        pr_err("%s:%d failed to move %d sma pages\n",
                __func__, __LINE__, rc);
        goto out;
    }

out:
    if (!swapwrite)
		current->flags &= ~PF_SWAPWRITE;

	return rc;
}
