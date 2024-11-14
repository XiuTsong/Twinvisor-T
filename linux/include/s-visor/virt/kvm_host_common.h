/*
 * @Date: 2024-11-14 14:54:19
 */

#ifndef __SVISOR_KVM_HOST_COMMON_H__
#define __SVISOR_KVM_HOST_COMMON_H__

#include <s-visor/common_defs.h>

#define TITANIUM_MAX_SUPPORTED_PHYSICAL_CORE_NUM SVISOR_PHYSICAL_CORE_NUM
#define TITANIUM_MAX_SIZE_PER_CORE (2048 + 64)

#ifndef __ASSEMBLER__

#include <linux/types.h>
#include <s-visor/virt/arm_vgic_common.h>

enum {
	REQ_KVM_TO_TITANIUM_FLUSH_IPA = 0,
	REQ_KVM_TO_TITANIUM_UNMAP_IPA,
	REQ_KVM_TO_TITANIUM_REMAP_IPA,
	REQ_KVM_TO_TITANIUM_BOOT,
	REQ_KVM_TO_TITANIUM_SHUTDOWN,
	REQ_KVM_TO_TITANIUM_MEMCPY
};

/* NOTE: KVM_SMC_UNMAP_IPA uses variable length of shared memory */
typedef struct {
	uint32_t sec_vm_id;
	uint32_t req_type;
	struct {
		uint64_t vttbr_el2;
		uint64_t vmpidr_el2;
		uint64_t vpidr_el2;
	} el2_regs;
	uint64_t icc_ich_lr[VGIC_V3_MAX_LRS]; // Pass icc_ich_lr<n> to s-vm
	union {
		/* No extra info for GENERAL */;
		struct {
			uint64_t qemu_s1ptp;
		} boot;
		struct {
			/* [start_pfn, start_pfn + nr_pages). Current granularity is 8M */
			uint64_t src_start_pfn;
			uint64_t dst_start_pfn;
			uint64_t nr_pages;
			/* ipn_list[0] -> src_start_pfn, ipn_list[1] -> src_start_pfn + 1 */
			uint64_t ipn_list[2048];
		} remap_ipa;
		struct {
			/* Tuples [start_pfn, end_pfn]. Maybe need a bitmap to batch? */
			uint64_t ipn_ranges[0];
		} unmap_ipa;
		struct {
			/* [start_pfn, start_pfn + nr_pages). Current granularity is 8M */
			uint64_t src_start_pfn;
			uint64_t dst_start_pfn;
			uint64_t nr_pages;
		} memcpy;
		/* No extra info for SHUTDOWN */;
	};
} kvm_smc_req_t;

#endif /* __ASSEMBLER__ */

#endif
