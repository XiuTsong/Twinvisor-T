/*
 * @Description: titanium mm.h
 * @Date: 2024-11-08 16:15:48
 */

#ifndef __SVISOR_MM_MM_H__
#define __SVISOR_MM_MM_H__

#include <s-visor/s-visor.h>

#define ROUNDUP(n,sz)   (((((n)-1)/(sz))+1)*(sz))
#define ROUNDDOWN(n,sz) ( (n) / (sz) * (sz) )

typedef unsigned long paddr_t;
typedef unsigned long vaddr_t;

extern unsigned long __secure_data linux_vp_offset;

#define VP_OFFSET ({ linux_vp_offset; })

#define __pa2va(x) ((unsigned long)((x) + VP_OFFSET))
#define __va2pa(x) ((unsigned long)((x) - VP_OFFSET))

#define pa2va __pa2va
#define va2pa __va2pa

void mm_primary_init(void);
void mm_secondary_init(void);

#endif
