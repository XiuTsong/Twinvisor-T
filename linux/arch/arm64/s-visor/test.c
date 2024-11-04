#include <linux/printk.h>
#include <s-visor/s-visor.h>

char stack[4096] __aligned(16) __secure_data;

void __secure_text test_s_visor(void)
{
    pr_info("test_s_visor\n");
}