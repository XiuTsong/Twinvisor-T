/*
 * @Description: titanium kvm_relay.h
 * @Date: 2024-11-14 14:52:04
 */

#ifndef __S_VISOR_KVM_RELAY_H__
#define __S_VISOR_KVM_RELAY_H__

#include <s-visor/virt/kvm_host_common.h>
#include <s-visor/virt/vm.h>

kvm_smc_req_t *get_smc_req_region_sec(unsigned int core_id);
int decode_kvm_vm_exit(struct titanium_state *state, uint32_t core_id, uint32_t vcpu_id);

#endif
