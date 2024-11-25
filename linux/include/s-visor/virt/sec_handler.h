/*
 * @Description: titanium sel1_handler.h
 * @Date: 2024-11-25 19:14:44
 */

#ifndef __SVISOR_SEC_HANDLER_H__
#define __SVISOR_SEC_HANDLER_H__

#include <s-visor/virt/vm.h>

void sel1_handle_dispatcher(struct titanium_vm *vm, uint32_t core_id, uint32_t vcpu_id);
#ifdef CONFIG_SEL1_OPT
void sel1_handle_write_ptes(struct titanium_vm *kvm_vm, uint32_t vcpu_id);
#endif

#endif
