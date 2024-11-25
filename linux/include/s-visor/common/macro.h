/*
 * @Description: titanium macro.h
 * @Date: 2024-11-18 15:14:52
 */

#ifndef __SVISOR_COMMON_MACRO_H__
#define __SVISOR_COMMON_MACRO_H__

#include <s-visor/lib/stdio.h>

#define _BUG_ON(expr, str) \
	do { \
		if ((expr)) { \
			printf("BUG: %s:%d %s\n", __func__, __LINE__, #expr); \
			for(;;) { \
			} \
		} \
	} while (0)

#define _BUG(str) \
	do { \
		printf("BUG: %s:%d %s\n", __func__, __LINE__, str); \
		for(;;) { \
		} \
	} while (0)

#define _ROUND_UP(x, n)		(((x) + (n) - 1) & ~((n) - 1))
#define _ROUND_DOWN(x, n)	((x) & ~((n) - 1))

#define _container_of(ptr, type, field) \
	((type *)((void *)(ptr) - (uint64_t)(&(((type *)(0))->field))))

#endif
