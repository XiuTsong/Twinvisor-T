/*
 * @Description: titanium buddy_allocator.h
 * @Date: 2024-11-08 16:01:05
 */

#ifndef __S_VISOR_BUDDY_ALLOCATOR_H__
#define __S_VISOR_BUDDY_ALLOCATOR_H__

#include <linux/types.h>

void bd_init(void);
void *bd_alloc(unsigned long nbytes, unsigned long alignment);
void bd_free(void* ptr);

// void shadow_bd_init(void);
// void *shadow_bd_alloc(unsigned long nbytes, unsigned long alignment);
// void shadow_bd_free(void *ptr);

#endif
