/*
 * @Description: titanium kvm_relay.c
 * @Date: 2024-11-14 14:51:50
 */

#include <s-visor/s-visor.h>
#include <s-visor/virt/kvm_relay.h>

#include <asm/asm-offsets.h>

extern uint64_t* __secure_data titanium_shared_pages;

kvm_smc_req_t * __secure_text get_smc_req_region_sec(unsigned int core_id)
{
	uint64_t *ptr = titanium_shared_pages + core_id * TITANIUM_MAX_SIZE_PER_CORE;
	/* First 32 entries are for guest gp_regs */
	return (kvm_smc_req_t *)(ptr + 32);
}
