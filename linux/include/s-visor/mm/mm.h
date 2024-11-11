/*
 * @Description: titanium mm.h
 * @Date: 2024-11-08 16:15:48
 */

#ifndef __SVISOR_MM_MM_H__
#define __SVISOR_MM_MM_H__

#define ROUNDUP(n,sz)   (((((n)-1)/(sz))+1)*(sz))
#define ROUNDDOWN(n,sz) ( (n) / (sz) * (sz) )

typedef unsigned long paddr_t;
typedef unsigned long vaddr_t;

vaddr_t pa2va(paddr_t phys);
paddr_t va2pa(vaddr_t virt);

void mm_primary_init(void);
void mm_secondary_init(void);

#endif
