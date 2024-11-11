/*
 * @Description: titanium sel1_shm.h
 * @Date: 2024-11-08 18:20:11
 */

#ifndef __SVISOR_SEL1_SHM_H__
#define __SVISOR_SEL1_SHM_H__

struct sec_shm {
    unsigned long exit_reason;
    unsigned long arg1;
    unsigned long arg2;
    unsigned long arg3;
    unsigned long guest_vector; /* offset == 0x20 */
    unsigned long pte_entry_idx;
    unsigned long irqnr; /* Inject interrupt use */
    unsigned long ret_val;
    unsigned long reserve_2;
};

#define GUEST_SHM_ADDR_BASE 0xffff800040000000
#define GUEST_SHM_SIZE 0x1000
#define GUEST_SHM_ADDR(core_id) (GUEST_SHM_ADDR_BASE + (core_id) * GUEST_SHM_SIZE)

#endif