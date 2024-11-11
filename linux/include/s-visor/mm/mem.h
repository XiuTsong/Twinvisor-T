/*
 * @Description: Describe memory range for buddy & page allocator
 * @Date: 2024-11-08 15:44:20
 */

#ifndef __SVISOR_MM_MEM_H__
#define __SVISOR_MM_MEM_H__

#include <linux/sizes.h>

extern char __svisor_mem_base[];

#define MEM_BASE ((unsigned long)__svisor_mem_base)


#define MEM_BD_ALLOC_SIZE SZ_8M
#define MEM_BD_ALLOC_BASE MEM_BASE
#define MEM_BD_ALLOC_END (MEM_BD_ALLOC_BASE + MEM_BD_ALLOC_SIZE)

#define MEM_PAGE_ALLOC_SIZE SZ_16M
#define MEM_PAGE_ALLOC_BASE MEM_BD_ALLOC_END
#define MEM_PAGE_ALLOC_END (MEM_PAGE_ALLOC_BASE + MEM_PAGE_ALLOC_SIZE)

#endif