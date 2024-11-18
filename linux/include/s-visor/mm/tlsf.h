/*
 * Created on 2024/11/05
 */

#ifndef __S_VISOR_TLSF_H__
#define __S_VISOR_TLSF_H__

#define _size_t unsigned long

/* tlsf_t: a TLSF structure. Can contain 1 to N pools. */
/* pool_t: a block of memory that TLSF can manage. */
typedef void* tlsf_t;
typedef void* pool_t;

/* Create/destroy a memory pool. */
tlsf_t tlsf_create(void* mem);
tlsf_t tlsf_create_with_pool(void* mem, _size_t bytes);
void tlsf_destroy(tlsf_t tlsf);
pool_t tlsf_get_pool(tlsf_t tlsf);

/* Add/remove memory pools. */
pool_t tlsf_add_pool(tlsf_t tlsf, void* mem, _size_t bytes);
void tlsf_remove_pool(tlsf_t tlsf, pool_t pool);

/* malloc/memalign/realloc/free replacements. */
void* tlsf_malloc(tlsf_t tlsf, _size_t bytes);
void* tlsf_memalign(tlsf_t tlsf, _size_t align, _size_t bytes);
void* tlsf_realloc(tlsf_t tlsf, void* ptr, _size_t size);
void tlsf_free(tlsf_t tlsf, void* ptr);

/* Returns internal block size, not original request size */
_size_t tlsf_block_size(void* ptr);

/* Overheads/limits of internal structures. */
_size_t tlsf_size(void);
_size_t tlsf_align_size(void);
_size_t tlsf_block_size_min(void);
_size_t tlsf_block_size_max(void);
_size_t tlsf_pool_overhead(void);
_size_t tlsf_alloc_overhead(void);

/* Debugging. */
typedef void (*tlsf_walker)(void* ptr, _size_t size, int used, void* user);
void tlsf_walk_pool(pool_t pool, tlsf_walker walker, void* user);
/* Returns nonzero if any internal consistency check fails. */
int tlsf_check(tlsf_t tlsf);
int tlsf_check_pool(pool_t pool);


#endif