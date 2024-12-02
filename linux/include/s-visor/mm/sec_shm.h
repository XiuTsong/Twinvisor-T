/*
 * @Description: titanium sel1_shm.h
 * @Date: 2024-11-08 18:20:11
 */

#ifndef __SVISOR_SEL1_SHM_H__
#define __SVISOR_SEL1_SHM_H__

#ifndef __ASSEMBLER__
struct sec_shm {
	union {
		unsigned long x0;
		unsigned long exit_reason;
	};
	union {
		unsigned long x1;
		unsigned long arg1;
	};
	union {
		unsigned long x2;
		unsigned long arg2;
	};
	union {
		unsigned long tpidr;
		unsigned long arg3;
	};
	unsigned long vbar_target;
	unsigned long ttbr_phys;
	unsigned long high_addr_offset;
	unsigned long guest_vector;
	unsigned long pte_entry_idx;
	unsigned long irqnr; /* Inject interrupt use */
	unsigned long ret_val;
};
#endif

#if CONFIG_ARM64_VA_BITS == 48
#define GUEST_SHM_ADDR_BASE 0xffff800040000000
#elif CONFIG_ARM64_VA_BITS == 39
#define GUEST_SHM_ADDR_BASE 0xffffff8040000000
#endif

#define GUEST_SHM_SIZE 0x1000
#define GUEST_SHM_ADDR(core_id) (GUEST_SHM_ADDR_BASE + (core_id) * GUEST_SHM_SIZE)

#endif