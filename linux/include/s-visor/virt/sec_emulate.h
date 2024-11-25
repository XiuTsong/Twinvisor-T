/*
 * @Date: 2024-11-25 18:51:09
 */

#ifndef __SVISOR_SEC_EMULATE_H__
#define __SVISOR_SEC_EMULATE_H__

#include <s-visor/virt/vm.h>

/* sel1 emulate related */
void emulate_access_trap(struct titanium_state *state, uint32_t core_id, uint32_t vcpu_id);
void emulate_sys_reg_start(struct titanium_state *state, unsigned long Rt, unsigned long Rt_val);
void emulate_sys_reg_finish(struct titanium_state *state);
int emulate_mmio_fault(struct titanium_state *state, unsigned long fault_ipa, uint64_t vcpu_id);

#endif