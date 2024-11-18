/*
 * @Description: titanium stage1_mmu.h
 * @Date: 2024-11-18 15:34:38
 */

#ifndef __SVISOR_MM_STAGE1_MMU_H__
#define __SVISOR_MM_STAGE1_MMU_H__

#include <asm/page-def.h>
#include <s-visor/lib/stdint.h>

#define PGTBL_4K_BITS                             (9)
#define PGTBL_4K_ENTRIES                          (1 << (PGTBL_4K_BITS))
#define PGTBL_4K_MAX_INDEX                        ((PGTBL_4K_ENTRIES) - 1)

#define ARM64_MMU_PTE_INVALID_MASK                (1 << 0)
#define ARM64_MMU_PTE_TABLE_MASK                  (1 << 1)

#define IS_PTE_INVALID(pte) (!((pte) & ARM64_MMU_PTE_INVALID_MASK))
#define IS_PTE_TABLE(pte) (!!((pte) & ARM64_MMU_PTE_TABLE_MASK))

#define BADDR_MASK ((1UL << 48) - 1)

#define PFN(paddr) ((paddr) >> PAGE_SHIFT)
#define VFN(vaddr) ((vaddr) >> PAGE_SHIFT)

#define PTE_NUM (1 << PAGE_ORDER)
/* FIXME: Only support 4kB granularity */
#define IS_VALID_LEVEL(l) ((l) >= 0 && (l) <= 3)

/* table format */
typedef union {
    struct {
        _uint64_t is_valid        : 1,
            is_table        : 1,
            ignored1        : 10,
            next_table_addr : 36,
            reserved1       : 4,
            ignored2        : 7,
            PXN             : 1,
            UXN              : 1,
            AP              : 2,
            NS              : 1;
    } table;
    struct {
        _uint64_t is_valid        : 1,
            is_table        : 1,
            AttrIndx        : 3,
            NS              : 1,
            AP              : 2,
            SH              : 2,
            AF              : 1,
            nG              : 1,
            OA              : 4,
            nT              : 1,
            reserved2       : 13,
            pfn             : 18,
            reserved3       : 3,
            DBM             : 1,
            Contiguous      : 1,
            PXN             : 1,
            UXN              : 1,
            soft_reserved   : 4,
            PBHA            : 4,   // Page based hardware attributes
            ignored         : 1;
    } l1_block;
    struct {
        _uint64_t is_valid        : 1,
            is_table        : 1,
            AttrIndx        : 3,
            NS              : 1,
            AP              : 2,
            SH              : 2,
            AF              : 1,
            nG              : 1,
            OA              : 4,
            nT              : 1,
            reserved2       : 4,
            pfn             : 27,
            reserved3       : 3,
            DBM             : 1,
            Contiguous      : 1,
            PXN             : 1,
            UXN              : 1,
            soft_reserved   : 4,
            PBHA            : 4,   // Page based hardware attributes
            ignored         : 1;
    } l2_block;
    struct {
        _uint64_t is_valid        : 1,
            is_page         : 1,
            AttrIndx        : 3,
            NS              : 1,
            AP              : 2,
            SH              : 2,
            AF              : 1,
            nG              : 1,
            pfn             : 36,
            reserved        : 3,
            DBM             : 1,
            Contiguous      : 1,
            PXN             : 1,
            UXN              : 1,
            soft_reserved   : 4,
            PBHA            : 4,   // Page based hardware attributes
            ignored         : 1;
    } l3_page;
    _uint64_t pte;
} s1_pte_t;

/* page_table_page type */
typedef struct {
    s1_pte_t ent[1 << PGTBL_4K_BITS];
} s1_ptp_t;

#endif
