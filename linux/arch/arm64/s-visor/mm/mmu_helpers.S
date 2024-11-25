#include <mm/mmu.h>
#include <common/asm.h>
#include <virt/sel1_defs.h>

.extern _boot_pt_l0_0
.extern _boot_pt_l0_1

LOCAL_FUNC_BEGIN(flush_dcache)
    dcache  cisw
    ret
LOCAL_FUNC_END(flush_dcache)

LOCAL_FUNC_BEGIN(invalidate_dcache)
    dcache  isw
    ret
LOCAL_FUNC_END(invalidate_dcache)

LOCAL_FUNC_BEGIN(invalidate_icache)
    ic      iallu
    dsb     nsh
    isb
    ret
LOCAL_FUNC_END(invalidate_icache)

BEGIN_FUNC(flush_dcache_and_tlb)
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp

    tlbi    alle2
    dsb     sy
    isb

    bl      flush_dcache

    ldp     x29, x30, [sp], #16
    ret
END_FUNC(flush_dcache_and_tlb)

.macro enable_mmu sctlr tmp
	mrs     \tmp, \sctlr
    /* Enable MMU */
	orr     \tmp, \tmp, #SCTLR_EL2_M
    /* Disable alignment checking */
	bic     \tmp, \tmp, #SCTLR_EL2_A
	bic     \tmp, \tmp, #SCTLR_EL2_SA
	/* Data accesses Cacheable */
        orr     \tmp, \tmp, #SCTLR_EL2_C
    /* Instruction access Cacheable */
	orr     \tmp, \tmp, #SCTLR_EL2_I
	msr     \sctlr, \tmp
	isb
.endm

.macro enable_mmu_el1 sctlr tmp
	mrs     \tmp, \sctlr
    /* Enable MMU */
	orr     \tmp, \tmp, #SCTLR_EL1_M
    /* Disable alignment checking */
	bic     \tmp, \tmp, #SCTLR_EL1_A
	bic     \tmp, \tmp, #SCTLR_EL1_SA
	/* Data accesses Cacheable */
        orr     \tmp, \tmp, #SCTLR_EL1_C
    /* Instruction access Cacheable */
	orr     \tmp, \tmp, #SCTLR_EL1_I
	msr     \sctlr, \tmp
	isb
.endm

.macro disable_mmu sctlr tmp
	mrs     \tmp, \sctlr
    /* Disable MMU */
	bic     \tmp, \tmp, #SCTLR_EL2_M
    /* Disable alignment checking */
	bic     \tmp, \tmp, #SCTLR_EL2_A
	bic     \tmp, \tmp, #SCTLR_EL2_SA
	bic     \tmp, \tmp, #SCTLR_EL2_C
    /* Disable Instruction Cache */
	bic     \tmp, \tmp, #SCTLR_EL2_I
	msr     \sctlr, \tmp
	isb
.endm

.macro disable_mmu_el1 sctlr tmp
	mrs     \tmp, \sctlr
    /* Disable MMU */
	bic     \tmp, \tmp, #SCTLR_EL1_M
    /* Disable alignment checking */
	bic     \tmp, \tmp, #SCTLR_EL1_A
	bic     \tmp, \tmp, #SCTLR_EL1_SA
	bic     \tmp, \tmp, #SCTLR_EL1_C
    /* Disable Instruction Cache */
	bic     \tmp, \tmp, #SCTLR_EL1_I
	msr     \sctlr, \tmp
	isb
.endm


BEGIN_FUNC(activate_mmu)
    /* We call nested functions, follow the ABI. */
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp

    bl      flush_dcache

    /* Ensure I-cache, D-cache and mmu are disabled for EL1/Stage1 */
    disable_mmu_el1 sctlr_el1 , x8

    /*
     * Invalidate the local I-cache so that any instructions fetched
     * speculatively are discarded.
     */
    bl      invalidate_icache

    /*
     *   DEVICE_nGnRnE      000     00000000
     *   DEVICE_nGnRE       001     00000100
     *   DEVICE_GRE         010     00001100
     *   NORMAL_NC          011     01000100
     *   NORMAL             100     11111111
     */
    ldr     x5, =MAIR(0x00, MT_DEVICE_nGnRnE) |\
                 MAIR(0x04, MT_DEVICE_nGnRE) |\
                 MAIR(0x0c, MT_DEVICE_GRE) |\
                 MAIR(0x44, MT_NORMAL_NC) |\
                 MAIR(0xff, MT_NORMAL)
    msr     mair_el1, x5

    ldr     x10, =TCR_TxSZ(48) | TCR_IRGN_WBWA | TCR_ORGN_WBWA | TCR_TG0_4K | TCR_TG1_4K | TCR_ASID16 | TCR_SHARED

    mrs     x9, ID_AA64MMFR0_EL1
    bfi     x10, x9, #32, #3
    msr     tcr_el1, x10

    /* Setup page tables */
    adrp    x8, _boot_pt_l0_0
    msr     ttbr0_el1, x8
//    adrp    x8, _boot_pt_l1_1
//    msr     ttbr1_el2, x8
    isb

    /* invalidate all TLB entries for EL2 */
    tlbi    vmalle1is
    dsb     ish
    isb

    enable_mmu_el1 sctlr_el1 , x8

    ldp     x29, x30, [sp], #16
    ret
END_FUNC(activate_mmu)
