/*
 * Created on 2024/11/06
 */

#include <s-visor/sched/smp.h>
#include <s-visor/s-visor.h>

extern unsigned int __get_core_pos(void);

unsigned int __secure_text get_core_id(void)
{
    return __get_core_pos();
}
