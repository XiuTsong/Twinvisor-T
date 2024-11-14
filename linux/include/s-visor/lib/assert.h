/*
 * @Date: 2024-11-12 18:23:44
 */

#ifndef __SVISOR_ASSERT_H__
#define __SVISOR_ASSERT_H__

static inline void __assert_fail(void)
{
    return;
}

static inline void __assert(int cond)
{
    if (cond != 0)
    {
        __assert_fail();
    }
}

#define assert(e)	__assert((e) ? 0 : 1)

#endif