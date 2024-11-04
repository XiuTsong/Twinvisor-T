/*
 * Secure Memory Allocator
 *
 * An allocator based on CMA.
 */

#include <linux/cma.h>
#include <linux/memblock.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/migrate.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/log2.h>
#include <linux/list_sort.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/dma-contiguous.h>
#include <linux/printk.h>
#include <linux/gfp.h>
#include <asm/atomic.h>
#include <asm/bitops.h>

#include <linux/sma.h>
#include <linux/list.h>
#include "cma.h"
#include "internal.h"

/* Utils */

bool sma_debug_flag = false;
EXPORT_SYMBOL(sma_debug_flag);

unsigned long migrate_times = 0;
unsigned long migrate_cycles = 0;
unsigned long topup_times = 0;
unsigned long topup_cycles = 0;
unsigned long topup_nr_pages = 0;
EXPORT_SYMBOL(topup_nr_pages);

struct sma global_sma;

static int sec_mem_cache_cmp(void *priv, 
        struct list_head *a, struct list_head *b) {
   struct sec_mem_cache *smc_a = 
       container_of(a, struct sec_mem_cache, node_for_pool); 
   struct sec_mem_cache *smc_b = 
       container_of(b, struct sec_mem_cache, node_for_pool);

   if (smc_a->base_pfn == smc_b->base_pfn)
       pr_err("%s:%d base_pfn: %lx == %lx\n", __func__, __LINE__, smc_a->base_pfn, smc_b->base_pfn);
   BUG_ON(smc_a->base_pfn == smc_b->base_pfn);

   return (smc_a->base_pfn < smc_b->base_pfn) ? -1 : 1;
}

static inline unsigned long avail_pages_after_compact(struct cma_pool *pool) {
    struct cma *cma = pool->cma;
    return (cma->base_pfn + cma->count) - 
        (pool->top_pfn - pool->nr_free_cache * SMA_CACHE_PAGES);
}

void sma_recycle_test(void) {
    struct cma_pool *pool = &global_sma.pools[DEFAULT_POOL];
    int i = 0;
    for (; i < 5; i++) {
        struct page *pages;
        unsigned long pfn;
        bool ret;
        pages = cma_alloc(pool->cma, SMA_CACHE_PAGES, SMA_CACHE_PG_ORDER, false);
        pfn = page_to_pfn(pages);
        ret = cma_release(pool->cma, pages, SMA_CACHE_PAGES);
        pr_info("%s [%d] alloc pfn: %lx, release ret: %d\n", __func__, i, pfn, ret);
    }
}

/* Initialization */

static struct sec_vm_info __dummy_svi;
static struct sec_vm_info *dummy_svi = &__dummy_svi;

void __init sec_mem_init_pools(phys_addr_t selected_size, 
        phys_addr_t selected_base, phys_addr_t limit, bool fixed) {
    struct cma_pool *pool = NULL;
    int ret;

    pr_info("%s:%d selected_size: %llx, selected_base: %llx\n",
            __func__, __LINE__, selected_size, selected_base);

    pool = &global_sma.pools[DEFAULT_POOL];
    ret = dma_contiguous_reserve_area(selected_size, selected_base, 
            0, &pool->cma, fixed);
    if (ret != 0) {
        pr_err("%s:%d Failed to reserve memory for SMA, ret = %d\n",
                __func__, __LINE__, ret);
    }
    pool->top_pfn = pool->cma->base_pfn;
    INIT_LIST_HEAD(&pool->used_cache_list);
    pool->nr_free_cache = 0;
    INIT_LIST_HEAD(&pool->free_cache_list);
    mutex_init(&pool->pool_lock);

    INIT_LIST_HEAD(&dummy_svi->inactive_cache_list);
    mutex_init(&dummy_svi->vm_lock);
}

static inline void reset_page_states(struct page *page)
{
    init_page_count(page);
    page_mapcount_reset(page);
    page->mapping = NULL;
    page->index = 0;
    page->flags &= ~PAGE_FLAGS_CHECK_AT_FREE;
}

/* Secure Memory Allocation */

bool sec_mem_reserve(unsigned long size, enum sec_pool_type sec_pool_type) {
    struct sec_mem_cache *current_cache;
    int i = 0;
    bool ret = true;
    
    mutex_lock(&dummy_svi->vm_lock);

    for (; i < size / SMA_CACHE_SIZE; i++) {
        current_cache = sec_mem_topup_cache(dummy_svi, dummy_svi->sec_pool_type);
        if (!current_cache) {
            pr_err("%s: failed to reserve %d-th cache\n", __func__, i);
            ret = false;
            break;
        }
        current_cache->is_active = false;
        list_add(&current_cache->node_for_vm, &dummy_svi->inactive_cache_list);
    }

    mutex_unlock(&dummy_svi->vm_lock);
    pr_info("%s: last cache base PFN: %lx\n", __func__, current_cache->base_pfn);
    return ret;
}
EXPORT_SYMBOL(sec_mem_reserve);

void sec_mem_clear(unsigned long size, enum sec_pool_type sec_pool_type) {
    struct cma_pool *target_pool = &global_sma.pools[sec_pool_type];
    struct list_head *free_head = &target_pool->free_cache_list;
    struct sec_mem_cache *smc_it, *next_it;
    unsigned long nr_free = 0;
    mutex_lock(&dummy_svi->vm_lock);
    /* Traverse each inactive cache */
    list_for_each_entry_safe(smc_it, next_it, &dummy_svi->inactive_cache_list, 
            node_for_vm) {
        if (sma_debug_flag)
            pr_info("%s: traverse cache PFN: %lx\n", __func__, smc_it->base_pfn); 
        list_del(&smc_it->node_for_vm);
        list_del(&smc_it->node_for_pool);
        list_add(&smc_it->node_for_pool, &target_pool->free_cache_list);
        target_pool->nr_free_cache++;
        nr_free++;
    }
    mutex_unlock(&dummy_svi->vm_lock);
    
    list_sort(NULL, free_head, sec_mem_cache_cmp);
    smc_it = list_first_entry(free_head, struct sec_mem_cache, node_for_pool);
    next_it = list_last_entry(free_head, struct sec_mem_cache, node_for_pool);
    pr_info("%s: clear %lu nr_free_cache: %lu, [%lx, %lx]\n", 
            __func__, nr_free, target_pool->nr_free_cache,
            smc_it->base_pfn, next_it->base_pfn);
}
EXPORT_SYMBOL(sec_mem_clear);

/**
 * sec_mem_topup_cache - topup secure memory cache for a secure VM
 * @owner_vm: the secure VM for which cache is allocated
 * @sec_pool_type: the memory type of cma_pool from which cache is allocated
 *
 * Policy: 1) use free_cache_list, 2) use cma_alloc. The list & owner of 
 * cache need to be updated.
 * 
 * The memory type of *owner_vm* can be different from *sec_pool_type*.
 */
struct sec_mem_cache *sec_mem_topup_cache(
        struct sec_vm_info *owner_vm, enum sec_pool_type sec_pool_type) {
    struct cma_pool *target_pool = &global_sma.pools[sec_pool_type];
    struct list_head *used_head;
    struct sec_mem_cache *ret = NULL;
    struct page *cache_pages = NULL;
    
    BUG_ON(!target_pool->cma);

    mutex_lock(&target_pool->pool_lock);
    used_head = &target_pool->used_cache_list;
    /* 
     * If has available cache in free_cache_list in target_cma, 
     * move it to used_cache_list, then return this cache.
     * FIXME: hold a lock of cma pool?
     */
    if (!list_empty(&target_pool->free_cache_list)) {
        struct list_head *free_head = &target_pool->free_cache_list;
        struct list_head *node = NULL;
        
        list_sort(NULL, free_head, sec_mem_cache_cmp);
        /* Get node with smallest *base_pfn* */
        ret = list_first_entry(free_head, struct sec_mem_cache, node_for_pool);
        node = &ret->node_for_pool;
        /* Delete node from free list & decrease nr_free_cache */
        list_del(node);
        target_pool->nr_free_cache--;
        /* Add node to used_cache_list */
        list_add(node, used_head);
        
        memset(ret->bitmap, 0, SMA_CACHE_BITMAP_SIZE);
        ret->owner_vm = owner_vm;
        /* Set as *active_cache* of *owner_vm* */
        owner_vm->active_cache = ret;
        ret->is_active = true;

        if (sma_debug_flag)
            pr_info("%s:%d REUSE CACHE: nr_free = %lu, type = %d, ret->base_pfn = %lx\n", 
                    __func__, __LINE__, target_pool->nr_free_cache, 
                    sec_pool_type, ret->base_pfn);

        mutex_unlock(&target_pool->pool_lock);
        return ret;
    }

    /* If no free cache, allocate SMA_CACHE_PAGES pages, always print failure */
    cache_pages = cma_alloc(target_pool->cma, SMA_CACHE_PAGES, 
            SMA_CACHE_PG_ORDER, false);
    if (!cache_pages) {
        mutex_unlock(&target_pool->pool_lock);
        return NULL;
    }
    
    ret = kmalloc(sizeof(struct sec_mem_cache), GFP_KERNEL);
    ret->base_pfn = page_to_pfn(cache_pages);
    /* Update allocated memory range of target CMA */
    BUG_ON(target_pool->top_pfn + SMA_CACHE_PAGES == ret->base_pfn);
    target_pool->top_pfn += SMA_CACHE_PAGES;
    ret->bitmap = kzalloc(SMA_CACHE_BITMAP_SIZE, GFP_KERNEL);
    if (!ret->bitmap) {
        BUG();
    }
    
    ret->sec_pool_type = sec_pool_type;
    /* Add node to used_cache_list */
    list_add(&ret->node_for_pool, used_head);
    
    ret->owner_vm = owner_vm;
    /* Set as *active_cache* of *owner_vm* */
    owner_vm->active_cache = ret;
    ret->is_active = true;
    
    /* Init *cache_lock* */
    mutex_init(&ret->cache_lock);
        
    if (sma_debug_flag) {
        pr_info("%s:%d NEW CACHE: type = %d, ret->base_pfn = %lx, cache_pages->_mapcount = %d\n", 
                __func__, __LINE__, sec_pool_type, ret->base_pfn, atomic_read(&cache_pages->_mapcount));
    }

    mutex_unlock(&target_pool->pool_lock);
    return ret;
}

struct page *sec_mem_alloc_page_local(struct sec_vm_info *owner_vm) {
    struct sec_mem_cache *current_cache;
    unsigned long bitmap_no, pfn;
    struct page *ret = NULL;
    
    /* FIXME: should we hold vm_lock throughout alloc? */
    mutex_lock(&owner_vm->vm_lock);
    current_cache = owner_vm->active_cache;

    if (!current_cache) {
        current_cache = sec_mem_topup_cache(owner_vm, owner_vm->sec_pool_type);
        if (!current_cache) {
            mutex_unlock(&owner_vm->vm_lock);
            return ret;
        }
    }

    mutex_lock(&current_cache->cache_lock);
    bitmap_no = find_next_zero_bit(current_cache->bitmap, SMA_CACHE_PAGES, 0);
    BUG_ON(bitmap_no >= SMA_CACHE_PAGES);
    
    bitmap_set(current_cache->bitmap, bitmap_no, 1);
    pfn = current_cache->base_pfn + bitmap_no;
    
    if (bitmap_full(current_cache->bitmap, SMA_CACHE_PAGES)) {
        if (sma_debug_flag)
            pr_info("%s:%d VM --> INACTIVE: current_cache->base_pfn = %lx\n", 
                    __func__, __LINE__, current_cache->base_pfn);

        /* Add the fullfilled cache to *inactive_cache_list* */
        list_add(&current_cache->node_for_vm, &owner_vm->inactive_cache_list);
        owner_vm->active_cache = NULL;
        current_cache->is_active = false;
    }
    mutex_unlock(&current_cache->cache_lock);

    mutex_unlock(&owner_vm->vm_lock);
    ret = pfn_to_page(pfn);
    if (page_count(ret) != 1) {
        pr_err("%s:%d ERROR pfn %lx refcount = %d\n", 
                __func__, __LINE__, page_to_pfn(ret), page_count(ret));
        /* Look at *get_page(new)* in remove_migration_pte */
        set_page_count(ret, 1);
    }
    ret->is_sec_mem = true;
    return ret;
}

struct page *sec_mem_alloc_page_other(struct sec_vm_info *owner_vm) {
    struct sec_mem_cache *current_cache = NULL;
    enum sec_pool_type sec_pool_type;
    unsigned long bitmap_no, pfn;
    struct page *ret = NULL;

    /* FIXME: what if another core set *active_cache*? */
    BUG_ON(owner_vm->active_cache);
    mutex_lock(&owner_vm->vm_lock);

    for (sec_pool_type = DEFAULT_POOL; sec_pool_type < SEC_POOL_TYPE_MAX; sec_pool_type++) {
        if (sec_pool_type == owner_vm->sec_pool_type)
            continue;

        current_cache = sec_mem_topup_cache(owner_vm, sec_pool_type);
        if (current_cache) {
            owner_vm->active_cache = current_cache;
            break;
        }
    }
    if (!current_cache) {
        mutex_unlock(&owner_vm->vm_lock);
        return ret;
    }

    mutex_lock(&current_cache->cache_lock);
    bitmap_no = find_next_zero_bit(current_cache->bitmap, SMA_CACHE_PAGES, 0);
    BUG_ON(bitmap_no >= SMA_CACHE_PAGES);
    
    bitmap_set(current_cache->bitmap, bitmap_no, 1);
    pfn = current_cache->base_pfn + bitmap_no;
    
    if (bitmap_full(current_cache->bitmap, SMA_CACHE_PAGES)) {
        /* Add the fullfilled cache to *inactive_cache_list* */
        list_add(&current_cache->node_for_vm, &owner_vm->inactive_cache_list);
        owner_vm->active_cache = NULL;
        current_cache->is_active = false;
    }
    mutex_unlock(&current_cache->cache_lock);

    mutex_unlock(&owner_vm->vm_lock);
    ret = pfn_to_page(pfn);
    if (page_count(ret) != 1) {
        pr_err("%s:%d ERROR pfn %lx refcount = %d\n", 
                __func__, __LINE__, page_to_pfn(ret), page_count(ret));
        /* Look at *get_page(new)* in remove_migration_pte */
        set_page_count(ret, 1);
    }
    ret->is_sec_mem = true;
    return ret;
}

/* Secure Memory Free */

void sec_mem_free_page(struct sec_vm_info *owner_vm, struct page *page) {
    /* Find the cache according to pfn, clear the bit in bitmap */
    struct sec_mem_cache *current_cache;
    unsigned long pfn = page_to_pfn(page);
    unsigned long cache_base_pfn = pfn & ~SMA_CACHE_PG_MASK;
    unsigned long bitmap_no = pfn - cache_base_pfn;
    bool from_inactive = false;

    mutex_lock(&owner_vm->vm_lock);
    current_cache = owner_vm->active_cache;

    /* Page is not in active cache */
    if (!current_cache || cache_base_pfn != current_cache->base_pfn) {
        struct sec_mem_cache *smc_it;
        current_cache = NULL;
        /* Traverse each inactive cache */
        list_for_each_entry(smc_it, &owner_vm->inactive_cache_list, 
                node_for_vm) {
            if (cache_base_pfn == smc_it->base_pfn) {
                current_cache = smc_it;
                from_inactive = true;
                break;
            }
        }
    }
    /* Panic if page not found in current VM */
    if (!current_cache) {
        pr_err("%s:%d Failed to free: pfn = %lx, cache_base_pfn = %lx\n", 
                __func__, __LINE__, pfn, cache_base_pfn);
        mutex_unlock(&owner_vm->vm_lock);
        return;
    }
    BUG_ON(!current_cache);
    
    mutex_lock(&current_cache->cache_lock);
    /* FIXME: shoud NOT free a freed page! */
    if (!test_bit(bitmap_no, current_cache->bitmap)) {
        pr_err("%s:%d ERROR pfn %lx already freed in this cache, refcount = %d\n", 
                __func__, __LINE__, page_to_pfn(page), page_count(page));
        mutex_unlock(&current_cache->cache_lock);
        mutex_unlock(&owner_vm->vm_lock);
        return;
    }
    
    reset_page_states(page);
    page->is_sec_mem = false;
    bitmap_clear(current_cache->bitmap, bitmap_no, 1);
    /* FIXME: sub refcount after *__move_sma_page* in *move_sma_page* cause BUG */
    if (page_ref_count(page) == 2) {
        put_page_testzero(page);
    }
    if (page_ref_count(page) != 1 || page_count(page) != 1) {
        pr_err("%s:%d ERROR pfn %lx count = %d:%d\n", 
                __func__, __LINE__, page_to_pfn(page), page_count(page), page_ref_count(page));
        set_page_count(page, 1);
    }
    
    if (bitmap_empty(current_cache->bitmap, SMA_CACHE_PAGES)) {
        /* 
         * This cache is free now, 1) remove it from used_cache_list,
         * 2) cma_release if it is the last cache, o.w. 3) add to 
         * free_cache_list of target_pool & add nr_free_cache. 
         */
        struct cma_pool *target_pool = 
            &global_sma.pools[current_cache->sec_pool_type];
        struct list_head *node = &current_cache->node_for_pool;

        /* 
         * Remove cache from inactive_cache_list if necessary,
         * o.w. set active_cache to NULL
         */
        if (from_inactive)
            list_del(&current_cache->node_for_vm);
        else {
            owner_vm->active_cache = NULL;
            current_cache->is_active = false;
        }
        
        mutex_lock(&target_pool->pool_lock);
        /* Remove cache from used_cache_list */
        list_del(node);

#if 0
        if (target_pool->top_pfn == (current_cache->base_pfn + SMA_CACHE_PAGES)) {
            bool ret;
            struct page *page = pfn_to_page(current_cache->base_pfn);
            if (sma_debug_flag)
                pr_info("%s:%d \t\t cma_release: current_cache->base_pfn = %lx, page->_refcount = %d:%d\n", 
                        __func__, __LINE__, current_cache->base_pfn, page_ref_count(page), page_count(page));
            ret = cma_release(target_pool->cma, page, SMA_CACHE_PAGES);
            if (!ret)
                BUG();
            target_pool->top_pfn -= SMA_CACHE_PAGES;
            /* Free current_cache */
            kfree(current_cache);
            /* TODO: release potential memory in *free_cache_list* */
        } else {
#endif
            if (sma_debug_flag)
                pr_info("%s:%d \t\t POOL --> FREE: top_pfn = %lx, base_pfn = %lx\n", 
                        __func__, __LINE__, target_pool->top_pfn, current_cache->base_pfn);
            list_add(node, &target_pool->free_cache_list);
            target_pool->nr_free_cache++;
#if 0
        }
#endif
        mutex_unlock(&target_pool->pool_lock);
    }
    mutex_unlock(&current_cache->cache_lock);
    mutex_unlock(&owner_vm->vm_lock);
}

/* Secure Memory Compaction */

static struct page *sec_mem_get_migrate_dst(struct page *page, 
        unsigned long private) {
    struct sec_mem_cache *dst_cache = (struct sec_mem_cache *)private;
    unsigned long dst_base_pfn = dst_cache->base_pfn;
    unsigned long page_offset = page_to_pfn(page) & SMA_CACHE_PG_MASK;
    struct page *dst_page = pfn_to_page(dst_base_pfn + page_offset);
    
    dst_page->is_sec_mem = true;
    if (page_count(dst_page) != 1) {
        pr_err("%s: ERROR dst pfn %lx refcount = %d, mapcount = %d\n", 
                __func__, page_to_pfn(dst_page), page_count(dst_page), 
                page_mapcount(dst_page));
    }
    
    //page->is_sec_mem = false;
    if (page_count(page) != 1) {
        pr_err("%s: ERROR src pfn %lx refcount = %d, mapcount = %d\n", 
                __func__, page_to_pfn(page), page_count(page), 
                page_mapcount(page));
    }
    
    return dst_page;
}

static void sec_mem_migrate_failure_callback(
        struct page *page, unsigned long private) {
    pr_err("%s: failed to migrate to pfn %lx, refcount = %d, mapcount = %d\n",
            __func__, page_to_pfn(page), page_count(page), 
            page_mapcount(page));
}

int sec_mem_compact_pool(enum sec_pool_type target_type) {
    struct cma_pool *target_pool;
    struct list_head *used_head, *free_head;
    struct sec_mem_cache *src_cache, *dst_cache;
    
    //extern int migrate_prep(void);
    //migrate_prep();
    
    target_pool = &global_sma.pools[target_type];

    mutex_lock(&target_pool->pool_lock);
    /* Prepare migrate src (largest PFN) to dst (smallest PFN) */
    used_head = &target_pool->used_cache_list;
    free_head = &target_pool->free_cache_list;
    list_sort(NULL, used_head, sec_mem_cache_cmp);
    list_sort(NULL, free_head, sec_mem_cache_cmp);

    while (!list_empty(free_head) && !list_empty(used_head)) {
        struct list_head src_page_list;
        unsigned long pfn_it;
        int ret;
        bool release_res;
        
        src_cache = list_last_entry(used_head, struct sec_mem_cache, node_for_pool);
        dst_cache = list_first_entry(free_head, struct sec_mem_cache, node_for_pool);
        /* If used_pfn < free_pfn, no need to migrate, release the free caches */
        if (src_cache->base_pfn < dst_cache->base_pfn)
            break;
        /* 
         * Remove src_cache from used_cache_list, 
         * remove dst_cache from free_cache_list
         */
        list_del(&src_cache->node_for_pool);
        list_del(&dst_cache->node_for_pool);
        target_pool->nr_free_cache--;
 
        INIT_LIST_HEAD(&src_page_list);
        for (pfn_it = src_cache->base_pfn;
                pfn_it < (src_cache->base_pfn + SMA_CACHE_PAGES); pfn_it++) {
            struct page *page = pfn_to_page(pfn_it);
            if (PageLRU(page)) {
                pr_err("%s:%d ERROR CMA %lx should NOT be LRU\n",
                        __func__, __LINE__, pfn_it);
            }
            if (page->is_sec_mem)
                list_add_tail(&page->lru, &src_page_list);
        }
        if (sma_debug_flag)
            pr_info("%s: before migrate_pages, src base = %lx, dst base = %lx\n",
                    __func__, src_cache->base_pfn, dst_cache->base_pfn);
        //ret = migrate_pages(&src_page_list, 
        ret = migrate_sma_pages(&src_page_list, 
                sec_mem_get_migrate_dst, sec_mem_migrate_failure_callback, 
                (unsigned long)dst_cache, MIGRATE_SYNC, MR_COMPACTION);
        if (ret != 0)
            pr_err("%s:\t migrate_pages ret = %d (nr_pages not migrated/error code)\n",
                    __func__, ret);
        for (pfn_it = src_cache->base_pfn;
                pfn_it < (src_cache->base_pfn + SMA_CACHE_PAGES); pfn_it++) {
            struct page *page = pfn_to_page(pfn_it);
            reset_page_states(page);
        }
        
        /* Copy the bitmap, add dst_cache to used_cache_list*/
        memcpy(dst_cache->bitmap, src_cache->bitmap, SMA_CACHE_BITMAP_SIZE);
        list_add(&dst_cache->node_for_pool, &target_pool->used_cache_list);
        
        /* 
         * Copy the owner of src_cache to dst_cache. 
         * FIXME: make sure all states are copied from src to dst
         */
        dst_cache->owner_vm = src_cache->owner_vm;
        dst_cache->is_active = src_cache->is_active;
        if (src_cache->is_active) {
            if (sma_debug_flag)
                pr_info("%s:%d \t\t active_cache from %lx to %lx\n",
                        __func__, __LINE__, src_cache->base_pfn, dst_cache->base_pfn); 
            src_cache->is_active = false;
            dst_cache->owner_vm->active_cache = dst_cache;
            dst_cache->is_active = true;
        } else {
            list_del(&src_cache->node_for_vm);
            list_add(&dst_cache->node_for_vm, 
                    &dst_cache->owner_vm->inactive_cache_list);
        }
#if 0        
        release_res = cma_release(target_pool->cma, 
                pfn_to_page(src_cache->base_pfn), SMA_CACHE_PAGES);
        target_pool->top_pfn -= SMA_CACHE_PAGES;
        if (sma_debug_flag)
            pr_info("%s:%d \t cma_release: res = %d, "
                    "top_pfn = %lx, base_pfn = %lx, page_count = %d:%d\n", 
                    __func__, __LINE__, 
                    release_res, target_pool->top_pfn, src_cache->base_pfn,
                    page_count(pfn_to_page(src_cache->base_pfn)),
                    page_ref_count(pfn_to_page(src_cache->base_pfn)));
        if (!release_res) {
            BUG();
        }
        /* Free source cache */
        kfree(src_cache);
#else
        list_add_tail(&src_cache->node_for_pool, &target_pool->free_cache_list);
        target_pool->nr_free_cache++;
#endif
    }
#if 0
    /* Migration complete, release rest of free caches if any */
    if (!list_empty(free_head)) {
        struct sec_mem_cache *smc_it, *next_it;
        /* Traverse each free cache */
        list_for_each_entry_safe(smc_it, next_it, free_head, node_for_pool) {
            bool release_res;
            list_del(&smc_it->node_for_pool);
            release_res = cma_release(target_pool->cma, 
                    pfn_to_page(smc_it->base_pfn), SMA_CACHE_PAGES);
            target_pool->top_pfn -= SMA_CACHE_PAGES;
            if (sma_debug_flag)
                pr_info("%s:%d \t cma_release: res = %d, top_pfn = %lx\n", 
                        __func__, __LINE__, release_res, target_pool->top_pfn);
            if (!release_res) {
                BUG();
            }
            /* Free this cache */
            kfree(smc_it);
        }
    }
#endif
    mutex_unlock(&target_pool->pool_lock);
    return 0;
}
EXPORT_SYMBOL(sec_mem_compact_pool);

static int sec_mem_async_compaction(void *args) {
    enum sec_pool_type sec_pool_type, target_type = (enum sec_pool_type)args;
    for (sec_pool_type = DEFAULT_POOL; sec_pool_type < SEC_POOL_TYPE_MAX; sec_pool_type++) {
        if (sec_pool_type == target_type)
            continue;
        sec_mem_compact_pool(sec_pool_type);
    }
    return 0;
}

int sec_mem_compaction(unsigned long req_nr_pages) {
    /* Choose the amplest cma_pool to compact */
    enum sec_pool_type sec_pool_type, target_type = SEC_POOL_TYPE_MAX;
    unsigned long largest_avail = 0, least_times = ~(0UL);
    
    for (sec_pool_type = DEFAULT_POOL; sec_pool_type < SEC_POOL_TYPE_MAX; sec_pool_type++) {
        struct cma_pool *pool = &global_sma.pools[sec_pool_type];
        /* FIXME: do we need pool_lock here? */
        unsigned long avail = avail_pages_after_compact(pool);
        if (largest_avail < avail) {
            largest_avail = avail;
            least_times = pool->nr_free_cache;
            target_type = sec_pool_type;
        } else if (largest_avail == avail && 
                pool->nr_free_cache > least_times) {
            least_times = pool->nr_free_cache;
            target_type = sec_pool_type;
        }
    }
    if (target_type == SEC_POOL_TYPE_MAX || 
            largest_avail < req_nr_pages) {
        return -ENOMEM;
    }

    sec_mem_compact_pool(target_type);

#if 0
    /* Spawn kthread to do async compaction */
    struct task_struct *async_compact_thread;
    async_compact_thread = kthread_run(sec_mem_async_compaction, 
            (void *)target_type, "%s", "sma-compact");
    if (IS_ERR(async_compact_thread))
        BUG();
    kthread_stop(async_compact_thread);
#endif

    return 0;
}

int sec_mem_compact_single_cache(enum sec_pool_type target_type) {
    struct cma_pool *target_pool;
    struct list_head *used_head, *free_head;
    struct sec_mem_cache *src_cache, *dst_cache;
    
    target_pool = &global_sma.pools[target_type];

    mutex_lock(&target_pool->pool_lock);
    /* Prepare migrate src (largest PFN) to dst (smallest PFN) */
    used_head = &target_pool->used_cache_list;
    free_head = &target_pool->free_cache_list;
    list_sort(NULL, used_head, sec_mem_cache_cmp);
    list_sort(NULL, free_head, sec_mem_cache_cmp);

    if (!list_empty(free_head) && !list_empty(used_head)) {
        struct list_head src_page_list;
        unsigned long pfn_it;
        int ret;
        bool release_res;
        
        src_cache = list_last_entry(used_head, struct sec_mem_cache, node_for_pool);
        dst_cache = list_first_entry(free_head, struct sec_mem_cache, node_for_pool);
        /* If used_pfn < free_pfn, no need to migrate, release the free caches */
        if (src_cache->base_pfn < dst_cache->base_pfn)
            return -1;
        /* 
         * Remove src_cache from used_cache_list, 
         * remove dst_cache from free_cache_list
         */
        list_del(&src_cache->node_for_pool);
        list_del(&dst_cache->node_for_pool);
        target_pool->nr_free_cache--;
 
        INIT_LIST_HEAD(&src_page_list);
        for (pfn_it = src_cache->base_pfn;
                pfn_it < (src_cache->base_pfn + SMA_CACHE_PAGES); pfn_it++) {
            struct page *page = pfn_to_page(pfn_it);
            if (PageLRU(page)) {
                pr_err("%s:%d ERROR CMA %lx should NOT be LRU\n",
                        __func__, __LINE__, pfn_it);
            }
            if (page->is_sec_mem)
                list_add_tail(&page->lru, &src_page_list);
        }
        if (sma_debug_flag)
            pr_info("%s: before migrate_pages, src base = %lx, dst base = %lx\n",
                    __func__, src_cache->base_pfn, dst_cache->base_pfn);
        ret = migrate_sma_pages(&src_page_list, 
                sec_mem_get_migrate_dst, sec_mem_migrate_failure_callback, 
                (unsigned long)dst_cache, MIGRATE_SYNC, MR_COMPACTION);
        if (ret != 0)
            pr_err("%s:\t migrate_pages ret = %d (nr_pages not migrated/error code)\n",
                    __func__, ret);
        
        /* Copy the bitmap, add dst_cache to used_cache_list*/
        memcpy(dst_cache->bitmap, src_cache->bitmap, SMA_CACHE_BITMAP_SIZE);
        list_add(&dst_cache->node_for_pool, &target_pool->used_cache_list);
        
        /* 
         * Copy the owner of src_cache to dst_cache. 
         * FIXME: make sure all states are copied from src to dst
         */
        dst_cache->owner_vm = src_cache->owner_vm;
        dst_cache->is_active = src_cache->is_active;
        if (src_cache->is_active) {
            if (sma_debug_flag)
                pr_info("%s:%d \t\t active_cache from %lx to %lx\n",
                        __func__, __LINE__, src_cache->base_pfn, dst_cache->base_pfn); 
            src_cache->is_active = false;
            dst_cache->owner_vm->active_cache = dst_cache;
            dst_cache->is_active = true;
        } else {
            list_del(&src_cache->node_for_vm);
            list_add(&dst_cache->node_for_vm, 
                    &dst_cache->owner_vm->inactive_cache_list);
        }
        
        release_res = cma_release(target_pool->cma, 
                pfn_to_page(src_cache->base_pfn), SMA_CACHE_PAGES);
        target_pool->top_pfn -= SMA_CACHE_PAGES;
        if (sma_debug_flag)
            pr_info("%s:%d \t cma_release: res = %d, "
                    "top_pfn = %lx, base_pfn = %lx, page_count = %d:%d\n", 
                    __func__, __LINE__, 
                    release_res, target_pool->top_pfn, src_cache->base_pfn,
                    page_count(pfn_to_page(src_cache->base_pfn)),
                    page_ref_count(pfn_to_page(src_cache->base_pfn)));
        if (!release_res) {
            BUG();
        }
        /* Free source cache */
        kfree(src_cache);
    }
    mutex_unlock(&target_pool->pool_lock);
    return 0;
}
