/*
 * Created on 2024/11/05
 */

#include <s-visor/s-visor.h>
#include <s-visor/common/lock.h>
#include <s-visor/mm/mem.h>
#include <s-visor/mm/page_allocator.h>

/* TODO: mem_base */
#define PG_SZ       0x1000UL
#define MAX_PAGE        (MEM_PAGE_ALLOC_SIZE / PG_SZ)

/*
 * TODO: Naive implementaion here
 */
__secure_data bool pages[MAX_PAGE];
__secure_data struct lock page_lock;

__secure_text void secure_page_alloc_init(void)
{
    memset((void *)MEM_PAGE_ALLOC_BASE, 0, MEM_PAGE_ALLOC_SIZE);
    memset(pages, 0, sizeof(pages));
    lock_init(&page_lock);
}

__secure_text static inline void* get_page_addr(int i)
{
    return (void *)(MEM_BASE + i * PG_SZ);
}

__secure_text static inline int get_page_index(void *ptr)
{
    return ((uint64_t)ptr - MEM_BASE) / PG_SZ;
}

__secure_text void *secure_page_alloc(void)
{
    void *ptr = NULL;
    uint32_t i;

    lock(&page_lock);
    for (i = 0U; i < MAX_PAGE; i++) {
        if (!pages[i]) {
            pages[i] = true;
            unlock(&page_lock);
            ptr = get_page_addr(i);
            return ptr;
        }
    }
    unlock(&page_lock);
    // printf("%s: OOM", __func__);

    return NULL;
}

__secure_text void secure_page_free(void *ptr)
{
    int i = get_page_index(ptr);

    lock(&page_lock);
    if (i < 0 || i >= MAX_PAGE) {
        // printf("%s: invalid ptr %p\n", __func__, ptr);
        unlock(&page_lock);
        return;
    }
    pages[i] = false;
    memset(ptr, 0, PG_SZ);
    unlock(&page_lock);
}
