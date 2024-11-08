/*
 * @Description: titanium buddy_allocator.c
 * @Date: 2024-11-08 15:35:05
 */

#include <s-visor/common/lock.h>
#include <s-visor/mm/tlsf.h>
#include <s-visor/mm/mem.h>
#include <s-visor/mm/buddy_allocator.h>
#include <s-visor/s-visor.h>

#define LEAF_SIZE       PAGE_SIZE

struct lock __secure_data secure_alloc_lock;
tlsf_t __secure_data secure_tlsf = NULL;

void __secure_text bd_init(void)
{
    memset((void *)MEM_BD_ALLOC_BASE, 0, MEM_BD_ALLOC_SIZE);
    lock_init(&secure_alloc_lock);
    secure_tlsf = tlsf_create_with_pool((void *)MEM_BD_ALLOC_BASE, MEM_BD_ALLOC_SIZE);
}

void __secure_text *bd_alloc(unsigned long nbytes, unsigned long alignment)
{
    void *align_ptr = NULL;
    lock(&secure_alloc_lock);
    align_ptr = tlsf_memalign(secure_tlsf, (1 << alignment), nbytes);
    if (!align_ptr || ((unsigned long)align_ptr & ((1 << alignment) - 1))) {
        // printf("OOM\n");
        asm volatile("b .");
    }
    unlock(&secure_alloc_lock);
    return align_ptr;
}

void __secure_text bd_free(void* ptr)
{
    lock(&secure_alloc_lock);
    tlsf_free(secure_tlsf, ptr);
    unlock(&secure_alloc_lock);
}
