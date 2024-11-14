/*
 * @Date: 2024-11-12 19:23:55
 */

#ifndef __SVISOR_EL3_PANIC_H_
#define __SVISOR_EL3_PANIC_H_

#include <linux/printk.h>

static inline void el3_panic(void)
{
    pr_info("el3 panic\n");
    while (1)
        ;
}

#endif
