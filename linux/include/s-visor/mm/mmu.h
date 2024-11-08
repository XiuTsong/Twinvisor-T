/*
 * Created on 2024/11/06
 */

#ifndef __SVISOR_MMU_H__
#define __SVISOR_MMU_H__

extern char __svisor_early_alloc_base[];
extern char __svisor_pg_dir[];
#define SECURE_PG_DIR_PHYS __pa_symbol((unsigned long)__svisor_pg_dir)

#endif