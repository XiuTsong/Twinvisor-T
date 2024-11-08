#ifndef __S_VISOR_PAGE_ALLOCATOR_H__
#define __S_VISOR_PAGE_ALLOCATOR_H__

void secure_page_alloc_init(void);
void *secure_page_alloc(void);
void secure_page_free(void *ptr);

#endif