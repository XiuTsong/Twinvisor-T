#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/migrate.h>
#include <linux/mm.h>
#include <linux/types.h>

extern int sec_mem_compact_pool(enum sec_pool_type target_type);

static int __init migrate_init(void)
{
    int ret;
    ret = sec_mem_compact_pool(0);
    pr_info("migate_pages ret: %d\n", ret);
    return ret;
}

static void __exit migrate_exit(void)
{
    printk("migrate_exit");
}

module_init(migrate_init);
module_exit(migrate_exit);

MODULE_LICENSE("GPL");
