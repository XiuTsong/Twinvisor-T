/*
 * @Description: trusted-firmware-a cpu_data.h
 * @Date: 2024-11-12 18:29:19
 */

#ifndef __SVISOR_CPU_DATA_H__
#define __SVISOR_CPU_DATA_H__

#define ARM_CACHE_WRITEBACK_SHIFT	6
/*
 * Some data must be aligned on the biggest cache line size in the platform.
 * This is known only to the platform as it might have a combination of
 * integrated and external caches.
 */
#define CACHE_WRITEBACK_GRANULE		(U(1) << ARM_CACHE_WRITEBACK_SHIFT)

#ifndef __ASSEMBLER__

#include <asm/sysreg.h>

#include <s-visor/common_defs.h>
#include <s-visor/lib/stdint.h>
#include <s-visor/lib/utils_def.h>
#include <s-visor/el3/platform.h>

/*******************************************************************************
 * Cache of frequently used per-cpu data:
 *   Pointers to non-secure and secure security state contexts
 *   Address of the crash stack
 * It is aligned to the cache line boundary to allow efficient concurrent
 * manipulation of these pointers on different cpus
 *
 * TODO: Add other commonly used variables to this (tf_issues#90)
 *
 * The data structure and the _cpu_data accessors should not be used directly
 * by components that have per-cpu members. The member access macros should be
 * used for this.
 ******************************************************************************/
typedef struct cpu_data {
	void *cpu_context[2];
} __aligned(CACHE_WRITEBACK_GRANULE) cpu_data_t;

extern cpu_data_t percpu_data[SVISOR_PHYSICAL_CORE_NUM];

/* Return the cpu_data structure for the current CPU. */
static inline struct cpu_data *_cpu_data(void)
{
    /* Note: we don't set it in tpidr */
	return (cpu_data_t *)&percpu_data[plat_my_core_pos()];
}

/**************************************************************************
 * APIs for initialising and accessing per-cpu data
 *************************************************************************/

void init_cpu_data_ptr(void);
void init_cpu_ops(void);

#define get_cpu_data(_m)		   _cpu_data()->_m
#define set_cpu_data(_m, _v)		   _cpu_data()->_m = (_v)
#define get_cpu_data_by_index(_ix, _m)	   _cpu_data_by_index(_ix)->_m
#define set_cpu_data_by_index(_ix, _m, _v) _cpu_data_by_index(_ix)->_m = (_v)
/* ((cpu_data_t *)0)->_m is a dummy to get the sizeof the struct member _m */
#define flush_cpu_data(_m)	   flush_dcache_range((uintptr_t)	  \
						&(_cpu_data()->_m), \
						sizeof(((cpu_data_t *)0)->_m))
#define inv_cpu_data(_m)	   inv_dcache_range((uintptr_t)	  	  \
						&(_cpu_data()->_m), \
						sizeof(((cpu_data_t *)0)->_m))
#define flush_cpu_data_by_index(_ix, _m)	\
				   flush_dcache_range((uintptr_t)	  \
					 &(_cpu_data_by_index(_ix)->_m),  \
						sizeof(((cpu_data_t *)0)->_m))

#endif /* __ASSEMBLER__ */

#endif /* __SVISOR_CPU_DATA_H__ */
