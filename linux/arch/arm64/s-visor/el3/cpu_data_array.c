/*
 * @Description: trusted-firmware-a cpu_data_array.c
 * @Date: 2024-11-12 18:38:39
 */

#include <s-visor/common_defs.h>
#include <s-visor/lib/el3_runtime/cpu_data.h>
#include <s-visor/el3/section.h>

/* The per_cpu_ptr_cache_t space allocation */
cpu_data_t __el3_data percpu_data[SVISOR_PHYSICAL_CORE_NUM];
