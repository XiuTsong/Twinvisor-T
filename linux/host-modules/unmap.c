#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/kvm_host.h>
#include <asm/kvm_host.h>
#include <linux/mm.h>
#include <linux/types.h>

extern void flush_titanium_shadow_page_tables(void);

static int __init unmap_init(void)
{
    int ret = 0;
    flush_titanium_shadow_page_tables();
    pr_info("titanium shadow page table flushed ret: %d\n", ret);
    return ret;
}

static void __exit unmap_exit(void)
{
    printk("migrate_exit");
}

module_init(unmap_init);
module_exit(unmap_exit);

MODULE_LICENSE("GPL");
