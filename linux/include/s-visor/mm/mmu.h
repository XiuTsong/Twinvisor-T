/*
 * Created on 2024/11/06
 */

#ifndef __SVISOR_MMU_H__
#define __SVISOR_MMU_H__

#include <linux/bits.h>

/* SCTLR_EL2 System Control Register aarch64 */

#define SCTLR_EL2_EE                BIT(25)     /* Endianness of data accesses at EL2, and stage 1 translation table walks in the EL2&0 translation regime */
#define SCTLR_EL2_WXN               BIT(19)     /* Write permission implies XN (Execute-never) */
#define SCTLR_EL2_I                 BIT(12)     /* Instruction access Cacheability control, for accesses at EL2 */
#define SCTLR_EL2_SA                BIT(3)      /* SP Alignment check */
#define SCTLR_EL2_C                 BIT(2)      /* Cacheability control for data accesses */
#define SCTLR_EL2_A                 BIT(1)      /* Alignment check enable */
#define SCTLR_EL2_M                 BIT(0)      /* MMU enable for EL2 stage 1 address translation */

/* SCTLR_EL1 System Control Register aarch64 */

#define SCTLR_EL1_EE                BIT(25)     /*  Endianness of data accesses at EL1, and stage 1 translation table walks in the EL1&0 translation regime */
#define SCTLR_EL1_WXN               BIT(19)     /* Write permission implies XN (Execute-never) */
#define SCTLR_EL1_I                 BIT(12)     /* Stage 1 instruction access Cacheability control, for accesses at EL0 and EL1 */
#define SCTLR_EL1_SA                BIT(3)      /* SP Alignment check */
#define SCTLR_EL1_C                 BIT(2)      /* Cacheability control for data accesses */
#define SCTLR_EL1_A                 BIT(1)      /* Alignment check enable */
#define SCTLR_EL1_M                 BIT(0)      /* MMU enable for EL2 stage 1 address translation */

#define TCR_T0SZ(x)       ((64 - (x)))
#define TCR_T1SZ(x)       ((64 - (x)) << 16)
#define TCR_TxSZ(x)       (TCR_T0SZ(x) | TCR_T1SZ(x))

#define TCR_IRGN0_WBWC    (1 << 8)
#define TCR_IRGN_NC       ((0 << 8) | (0 << 24))
#define TCR_IRGN_WBWA     ((1 << 8) | (1 << 24))
#define TCR_IRGN_WT       ((2 << 8) | (2 << 24))
#define TCR_IRGN_WBnWA    ((3 << 8) | (3 << 24))
#define TCR_IRGN_MASK     ((3 << 8) | (3 << 24))

#define TCR_ORGN0_WBWC    (1 << 10)
#define TCR_ORGN_NC       ((0 << 10) | (0 << 26))
#define TCR_ORGN_WBWA     ((1 << 10) | (1 << 26))
#define TCR_ORGN_WT       ((2 << 10) | (2 << 26))
#define TCR_ORGN_WBnWA    ((3 << 10) | (3 << 26))
#define TCR_ORGN_MASK     ((3 << 10) | (3 << 26))

#define TCR_SH0_ISH       (3 << 12)

#define TCR_TG0_4K        (0 << 14)
#define TCR_TG0_64K       (1 << 14)
#define TCR_TG1_4K        (2 << 30)
#define TCR_TG1_64K       (3 << 30)

#define TCR_PS_4G         (0 << 16)
#define TCR_PS_64G        (1 << 16)
#define TCR_PS_1T         (2 << 16)
#define TCR_PS_4T         (3 << 16)
#define TCR_PS_16T        (4 << 16)
#define TCR_PS_256T       (5 << 16)

/* bits are reserved as 1 */
#define TCR_EL2_RES1      ((1 << 23) | (1 << 31))
#define TCR_ASID16        (1UL << 36)

#define UL(x) x##UL

#define TCR_SH0_SHIFT 12
#define TCR_SH0_MASK (UL(3) << TCR_SH0_SHIFT)
#define TCR_SH0_INNER (UL(3) << TCR_SH0_SHIFT)
#define TCR_SH1_SHIFT 28
#define TCR_SH1_MASK (UL(3) << TCR_SH1_SHIFT)
#define TCR_SH1_INNER (UL(3) << TCR_SH1_SHIFT)

#define TCR_SHARED (TCR_SH0_INNER | TCR_SH1_INNER)

#define TCR_TBI0 (UL(1) << 37)
#define TCR_A1   (UL(1) << 22)


#define ID_AA64PFR0_EL2_GIC     (0b1111 << 24)

#define MT_DEVICE_nGnRnE  0
#define MT_DEVICE_nGnRE   1
#define MT_DEVICE_GRE     2
#define MT_NORMAL_NC      3
#define MT_NORMAL         4
#define MAIR(_attr, _mt)  ((unsigned long)(_attr) << ((_mt) * 8))

#ifndef __ASSEMBLER__

extern char __svisor_early_alloc_base[];
extern char __svisor_pg_dir[];
extern char __svisor_idmap_dir[];
#define SECURE_PG_DIR_VIRT ((unsigned long)__svisor_pg_dir)
#define SECURE_PG_DIR_PHYS __pa_symbol((unsigned long)__svisor_pg_dir)
#define SECURE_IDMAP_DIR_PHYS __pa_symbol((unsigned long)__svisor_idmap_dir)

#endif

#endif