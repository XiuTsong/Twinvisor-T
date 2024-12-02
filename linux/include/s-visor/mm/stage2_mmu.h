/*
 * @Description: titanium stage2_mmu.h
 * @Date: 2024-11-18 15:45:58
 */

#ifndef __SVISOR_MM_STAGE2_MMU_H__
#define __SVISOR_MM_STAGE2_MMU_H__

#include <s-visor/lib/stdint.h>

#define ARM64_MMU_PTE_INVALID_MASK                (1 << 0)
#define ARM64_MMU_PTE_TABLE_MASK                  (1 << 1)

#define PAGE_ORDER                                (9)
#define ARM64_LEVEL_INDEX_MASK  ((1UL << PAGE_ORDER) - 1)

#define PGTBL_4K_BITS                             (9)
#define PGTBL_4K_ENTRIES                          (1 << (PGTBL_4K_BITS))
#define PGTBL_4K_MAX_INDEX                        ((PGTBL_4K_ENTRIES) - 1)

#define ARM64_MMU_L0_SHIFT                        (27)
#define ARM64_MMU_L1_BLOCK_ORDER                  (18)
#define ARM64_MMU_L2_BLOCK_ORDER                  (9)
#define ARM64_MMU_L3_PAGE_ORDER                   (0)

#define ARM64_MMU_L0_BLOCK_PAGES  (PTP_ENTRIES * ARM64_MMU_L1_BLOCK_PAGES)
#define ARM64_MMU_L1_BLOCK_PAGES  (1UL << ARM64_MMU_L1_BLOCK_ORDER)
#define ARM64_MMU_L2_BLOCK_PAGES  (1UL << ARM64_MMU_L2_BLOCK_ORDER)
#define ARM64_MMU_L3_PAGE_PAGES   (1UL << ARM64_MMU_L3_PAGE_ORDER)

#define L0_PER_ENTRY_PAGES  (ARM64_MMU_L0_BLOCK_PAGES)
#define L1_PER_ENTRY_PAGES  (ARM64_MMU_L1_BLOCK_PAGES)
#define L2_PER_ENTRY_PAGES        (ARM64_MMU_L2_BLOCK_PAGES)
#define L3_PER_ENTRY_PAGES  (ARM64_MMU_L3_PAGE_PAGES)

#define ARM64_MMU_L1_BLOCK_SIZE   (ARM64_MMU_L1_BLOCK_PAGES << PAGE_SHIFT)
#define ARM64_MMU_L2_BLOCK_SIZE   (ARM64_MMU_L2_BLOCK_PAGES << PAGE_SHIFT)
#define ARM64_MMU_L3_PAGE_SIZE    (ARM64_MMU_L3_PAGE_PAGES << PAGE_SHIFT)

#define ARM64_MMU_L1_BLOCK_MASK   (ARM64_MMU_L1_BLOCK_SIZE - 1)
#define ARM64_MMU_L2_BLOCK_MASK   (ARM64_MMU_L2_BLOCK_SIZE - 1)
#define ARM64_MMU_L3_PAGE_MASK    (ARM64_MMU_L3_PAGE_SIZE - 1)

#define GET_VA_OFFSET_L1(va)      (va & ARM64_MMU_L1_BLOCK_MASK)
#define GET_VA_OFFSET_L2(va)      (va & ARM64_MMU_L2_BLOCK_MASK)
#define GET_VA_OFFSET_L3(va)      (va & ARM64_MMU_L3_PAGE_MASK)

#define GET_IPN_OFFSET_L0(ipn)  (((ipn) >> ARM64_MMU_L0_SHIFT) & ARM64_LEVEL_INDEX_MASK)
#define GET_IPN_OFFSET_L1(ipn)  (((ipn) >> ARM64_MMU_L1_BLOCK_ORDER) & ARM64_LEVEL_INDEX_MASK)
#define GET_IPN_OFFSET_L2(ipn)  (((ipn) >> ARM64_MMU_L2_BLOCK_ORDER) & ARM64_LEVEL_INDEX_MASK)
#define GET_IPN_OFFSET_L3(ipn)  (((ipn) >> ARM64_MMU_L3_PAGE_ORDER) & ARM64_LEVEL_INDEX_MASK)

#define PTE_DESCRIPTOR_INVALID                    (0)
#define PTE_DESCRIPTOR_BLOCK                      (1)
#define PTE_DESCRIPTOR_TABLE                      (3)
#define PTE_DESCRIPTOR_MASK                       (3)

/* PAGE TABLE PAGE TYPE */
#define TABLE_TYPE              1
#define BLOCK_TYPE              2

typedef union {
    struct {
        _uint64_t is_valid        : 1,
            is_table        : 1,
            ignored1        : 10,
            next_table_addr : 36,
            reserved1       : 4,
            ignored2        : 7,
            reserved2       : 5;
    } table;
    struct {
        _uint64_t is_valid        : 1,
            is_table        : 1,
            mem_attr        : 4,   // Memory attributes index
            S2AP            : 2,   // Data access permissions
            SH              : 2,   // Shareability
            AF              : 1,   // Accesss flag
            zero            : 1,
            reserved1       : 4,
            nT              : 1,
            reserved2       : 13,
            pfn             : 18,
            reserved3       : 3,
            DBM             : 1,   // Dirty bit modifier
            Contiguous      : 1,
            XN              : 2,
            soft_reserved   : 4,
            PBHA            : 4,   // Page based hardware attributes
            reserved4       : 1;
    } l1_block;
    struct {
        _uint64_t is_valid        : 1,
            is_table        : 1,
            mem_attr        : 4,   // Memory attributes index
            S2AP            : 2,   // Data access permissions
            SH              : 2,   // Shareability
            AF              : 1,   // Accesss flag
            zero            : 1,
            reserved1       : 4,
            nT              : 1,
            reserved2       : 4,
            pfn             : 27,
            reserved3       : 3,
            DBM             : 1,   // Dirty bit modifier
            Contiguous      : 1,
            XN              : 2,   // Execute never
            soft_reserved   : 4,
            PBHA            : 4,   // Page based hardware attributes
            reserved4       : 1;
    } l2_block;
    struct {
        _uint64_t is_valid        : 1,
            is_page         : 1,
            mem_attr        : 4,   // Memory attributes index
            S2AP            : 2,   // Data access permissions
            SH              : 2,   // Shareability
            AF              : 1,   // Accesss flag
            zero            : 1,
            pfn             : 36,
            reserved        : 3,
            DBM             : 1,   // Dirty bit modifier
            Contiguous      : 1,
            XN              : 2,   // Execute never
            soft_reserved   : 4,
            PBHA            : 4,   // Page based hardware attributes
            reserved2       : 1;
    } l3_page;
    _uint64_t pte;
} pte_t;

/* page_table_page type */
typedef struct {
    pte_t ent[1 << PGTBL_4K_BITS];
} ptp_t;

pte_t translate_stage2_pt(paddr_t s2pt_phys, paddr_t ipn);

#endif
