#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/mm_types.h>
#include <asm/pgtable.h>

extern struct device secure_memory_dev;
extern struct page *dma_alloc_from_contiguous(struct device *,
        size_t, unsigned int, bool);
extern struct page *dma_release_from_contiguous(struct device *,
        struct page *, int);
extern bool sec_mem_reserve(unsigned long size,
        enum sec_pool_type sec_pool_type);
extern bool sec_mem_clear(unsigned long size,
        enum sec_pool_type sec_pool_type);

struct page *rsv_pages;
unsigned long size = 512 << 20;

static int __init reserve_init(void)
{
    int ret;
    //rsv_pages = dma_alloc_from_contiguous(&secure_memory_dev,
    //        size >> PAGE_SHIFT, get_order(size), false);
    ret = sec_mem_reserve(size, 0) ? 0 : -1;
    pr_info("dma_alloc_from_contiguous ret: %d\n", ret);
    return ret;
}

static void __exit reserve_exit(void)
{
    //unsigned long nr_pages = size >> PAGE_SHIFT;
    //ret = dma_release_from_contiguous(&secure_memory_dev, rsv_pages, nr_pages);
    sec_mem_clear(size, 0);
    pr_info("dma_release_from_contiguous\n");
}

module_init(reserve_init);
module_exit(reserve_exit);

MODULE_LICENSE("GPL");
