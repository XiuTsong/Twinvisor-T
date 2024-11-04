#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/migrate.h>
#include <linux/mm.h>
#include <linux/types.h>

extern struct list_head test_vm_pages;
extern struct list_head free_vm_pages;

extern int migrate_pages(struct list_head *l, new_page_t new, free_page_t free,
		unsigned long private, enum migrate_mode mode, int reason);

struct page *test_migrate_target(struct page *page, unsigned long private)
{
    unsigned long from_pfn = page_to_pfn(page);
    unsigned long mask_512m = 0x1ffff; 
    unsigned long to_base = 0x3000000; 
    struct page *to_page = pfn_to_page(to_base + (from_pfn & mask_512m));
    get_page(page);
    get_page(to_page);
    return to_page;
}

void test_migrate_free(struct page *page, unsigned long private)
{
    list_del(&page->lru);
    list_add(&page->lru, &free_vm_pages);
}

static int __init cma_mig_init(void)
{
    int ret;
    ret = migrate_pages(&test_vm_pages, test_migrate_target, test_migrate_free, 0,
            MIGRATE_SYNC, MR_COMPACTION);
    pr_info("migate_pages ret: %d\n", ret);
    return ret;
}

static void __exit cma_mig_exit(void)
{
    printk("cma_mig_exit");
}

module_init(cma_mig_init);
module_exit(cma_mig_exit);

MODULE_LICENSE("GPL");
