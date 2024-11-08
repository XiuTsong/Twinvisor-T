/*
 * Created on 2024/11/06
 */

#include <s-visor/sched/smp.h>

extern unsigned int __get_core_pos(void);

unsigned int get_core_id(void)
{
    return __get_core_pos();
}
