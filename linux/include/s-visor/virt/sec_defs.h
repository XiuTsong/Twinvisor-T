/*
 * @Description: titanium sel1_defs.h
 * @Date: 2024-11-08 18:26:58
 */

#ifndef __SVISOR_SEC_DEFS_H__
#define __SVISOR_SEC_DEFS_H__

#define GATE_ENTER_IMM 0x998
#define GATE_EXIT_IMM 0x999

#define GATE_ENTER_ADDR ((unsigned long)0x200000000)
#define GATE_EXIT_ADDR ((unsigned long)0x200001000)
#define GATE_SHARE_MEM_ADDR ((unsigned long)0x200002000)

#define GUEST_GATE_ENTER_ADDR ((unsigned long)0xffff800030000000)
#define GUEST_GATE_EXIT_ADDR ((unsigned long)0xffff800030001000)
#define SHARED_MEM_SIZE PAGE_SIZE

#define SEL1_VSTTBR ((unsigned long)0x400000000)

/* SEL1_SVC: Call gate to s-visor, and then return back */
#define SEL1_SVC                        0x99

#define SEL1_EXIT_WFI                   1
#define SEL1_EXIT_WRITE_TTBR0           2
#define SEL1_EXIT_CREATE_TTBR0          3
#define SEL1_EXIT_SWITCH_TTBR0          4
#define SEL1_EXIT_WRITE_TTBR1           5
#define SEL1_EXIT_WRITE_SCTLR           6
#define SEL1_EXIT_WRITE_TCR             7
#define SEL1_EXIT_WRITE_MAIR            8
#define SEL1_EXIT_WRITE_VBAR            9
#define SEL1_EXIT_ENABLE_DAIF           10
#define SEL1_EXIT_RESTORE_DAIF          11
#define SEL1_EXIT_READ_MIDR             12
#define SEL1_EXIT_READ_MPIDR            13

#define SEL1_EXIT_HANDLE_WRITE_PGTBL    20
#define SEL1_EXIT_HANDLE_PGFAULT        21
#define SEL1_EXIT_DESITROY_TTBR0        22
#define SEL1_EXIT_INIT_SWIOTLB          23
#define SEL1_EXIT_WRITE_PTES            24
#define SEL1_EXIT_WRITE_PTE             25
#define SEL1_EXIT_SCHED                 26

#define SEL1_EXIT_TEST_MEM_ABORT        96
#define SEL1_EXIT_HANDLE_IPI            97
#define SEL1_EXIT_END_GIC_HANDLE        98
#define SEL1_EXIT_TEST                  99

#define START_LOG                       0x97
#define PRINT_LOG                       0x98

/*
 * SEL1_SYS64: System access trap, need to go to n-visor
 * FIXME: Complete all registers that requier access trapping
 */
#define SEL1_SYS64                      0x100
#define SEL1_WRITE_ICC_SGI1R_EL1          1

// #define CONFIG_SEL1_OPT

#endif
