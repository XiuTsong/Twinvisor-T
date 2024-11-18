/*
 * @Description: tianium stdint.h
 * @Date: 2024-11-11 19:21:32
 */

#ifndef __SVISOR_STDINT_H__
#define __SVISOR_STDINT_H__

#include <linux/types.h>

#ifndef __ASSEMBLER__

typedef long register_t;
typedef unsigned long u_register_t;
typedef unsigned long paddr_t;
typedef unsigned long vaddr_t;

#define _uint64_t unsigned long long
#define _uint32_t unsigned int

#endif

#endif