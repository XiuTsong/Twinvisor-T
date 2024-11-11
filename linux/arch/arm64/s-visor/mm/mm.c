/*
 * @Description: titanium mm.c
 * @Date: 2024-11-08 15:32:36
 */
#include <asm/page-def.h>
#include <linux/memory.h>

#include <s-visor/s-visor.h>
#include <s-visor/mm/buddy_allocator.h>
#include <s-visor/mm/page_allocator.h>
#include <s-visor/mm/mm.h>

#ifdef SVISOR_DEBUG
#pragma GCC optimize("O0")
#endif

uint64_t __secure_data current_cpu_stack_sps[SVISOR_PHYSICAL_CORE_NUM] = {0};

vaddr_t __secure_text pa2va(paddr_t phys)
{
    return (vaddr_t)__phys_to_virt(phys);
}

paddr_t __secure_text va2pa(vaddr_t virt)
{
    return (paddr_t)__virt_to_phys(virt);
}


static void __secure_text init_percpu_stack(void)
{
    unsigned int i = 0U;

    for (; i < SVISOR_PHYSICAL_CORE_NUM; i++) {
        uint64_t stack_page = (uint64_t)bd_alloc(PAGE_SIZE, 12);
        current_cpu_stack_sps[i] = stack_page + PAGE_SIZE;
    }
}

void __secure_text mm_primary_init(void)
{
    /*
     * n-visor has prepared pgtable for s-visor
     * So we only need to initialize memory allocator.
     */
    bd_init();
    // shadow_bd_init();
    secure_page_alloc_init();
    init_percpu_stack();
}
