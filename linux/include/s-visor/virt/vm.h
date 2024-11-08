/*
 * @Description: titanium vm.h
 * @Date: 2024-11-08 16:25:20
 */

#ifndef __S_VISOR_VM_H__
#define __S_VISOR_VM_H__

#include <s-visor/virt/vcpu.h>

#define MAX_VCPU_NUM 8

enum vm_state {
    VS_CREATE = 0,
    VS_INIT,
    VS_READY,
    VS_RUNNING,
    VS_DESTROY,
};

enum vm_type {
    VT_TRUSTED = 0,
    VT_KVM,
};

struct titanium_vm_ops {
   int (*smc_handler)(struct titanium_state *);
};

struct titanium_vm {
    int vm_id;                                       // VM ID
    char *vm_name;                                   // VM name, used for debugging

    struct list_head vm_list_node;                   // link all vm in the same list

    struct titanium_vcpu *vcpus[MAX_VCPU_NUM];       // all VCPUs
    struct thread_vector_table *thread_vector_table; // Trusted OS callback table
    struct titanium_vm_ops *vm_ops;                   // VM specific operations

    int vm_state;
    int vm_type;

    /* n-visor vttbr, passed by shared memory */
    unsigned long vttbr_el2;
    unsigned long vpidr_el2;

    /* guest vector table address */
    unsigned long guest_vector;

    unsigned int boot_core_id;
    bool is_booted;

//    /* All vcpu->s1mmu share the same ttbr1 spt */
//    struct spt ttbr1_spt;
//
//    /* spt information */
//    struct spt_info spt_info;
//
//    /* per-core shared memory with guest */
//    struct sec_shm *shms[PHYSICAL_CORE_NUM];
};


void init_vms(void);
// struct titanium_vm *get_vm_by_id(int vm_id);
// void cold_boot_vms(void);
struct titanium_state *this_core_titanium_state(void);
// struct titanium_state *get_titanium_state(uint64_t core_id);


#endif