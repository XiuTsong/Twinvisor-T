/*
 * @Description: titanium sched.c
 * @Date: 2024-11-08 16:30:10
 */

#include <s-visor/sched/sched.h>

static struct list_head *titanium_vm_list;

void tianium_vm_sched_init(void)
{
//    printf("%s called\n", __func__);
}

void tianium_vm_sched(void)
{
    /**
     * No scheduling algorithm implemented here right now
     * We just use OPTEE to be the first VM.
     */
}