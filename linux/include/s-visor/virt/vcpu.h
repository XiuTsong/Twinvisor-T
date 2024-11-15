/*
 * @Description: titanium vcpu.h
 * @Date: 2024-11-08 14:58:47
 */

#ifndef __S_VISOR_VCPU_H__
#define __S_VISOR_VCPU_H__

#include <linux/types.h>

#include <s-visor/s-visor.h>
#include <s-visor/mm/sec_shm.h>

enum vcpu_state {
    VCPU_INIT = 0,
    VCPU_READY,
    VCPU_TRAPPED,
    VCPU_RUNNING,
    VCPU_DESTROY,
};

struct gp_regs {
	unsigned long x[30];
	unsigned long lr;
	unsigned long pc;
};

struct sys_regs {
	unsigned long spsr;
	unsigned long elr;
	unsigned long sctlr;
	unsigned long sp;
	unsigned long sp_el0;
	unsigned long esr;
	unsigned long ttbr0;
	unsigned long ttbr1;
	unsigned long vbar;
	unsigned long mair;
	unsigned long amair;
	unsigned long tcr;
	unsigned long tpidr;
	unsigned long far;
};

struct vcpu_ctx {
    struct gp_regs gp_regs;
    struct sys_regs sys_regs;
};

struct titanium_host_regs {
    struct gp_regs gp_regs;
    struct sys_regs sys_regs;
};

struct thread_vector_table {
	unsigned int std_smc_entry;
	unsigned int fast_smc_entry;
	unsigned int cpu_on_entry;
	unsigned int cpu_off_entry;
	unsigned int cpu_resume_entry;
	unsigned int cpu_suspend_entry;
	unsigned int fiq_entry;
	unsigned int system_off_entry;
	unsigned int system_reset_entry;
};

struct titanium_entry_helper {
    unsigned long guest_ttbr0_el1;
    unsigned long guest_ttbr1_el1;
    bool jump_to_guest_vbar;
    unsigned long vbar_target;
};

struct titanium_sys_reg_emulate {
    bool is_emulating;
    unsigned int Rt;
    unsigned long orig_val;
};

struct titanium_state {
    struct titanium_host_regs host_state;
    struct vcpu_ctx current_vcpu_ctx;
    struct titanium_vm *current_vm;
    int current_vcpu_id;
    unsigned long ret_lr;
    struct sec_shm *shared_mem;

    /* For guest-entry.S use */
    struct titanium_entry_helper entry_helper;
    struct titanium_sys_reg_emulate emulate;
    // struct titanium_vgic vgic;
};

/* titanium_decode is the same as kvm_decode */
struct titanium_decode {
    unsigned long rt;
    int sign_extend;
	/* Witdth of the register accessed by the faulting instruction is 64-bits */
    int sixty_four;
};

struct titanium_vcpu {
    struct titanium_vm *vm; // Point to vm struct the vcpu belongs to
    int vcpu_id;
    int vcpu_state;

    /* KVM VM only */
    int is_s2pt_violation;
    unsigned long fault_ipn;

    struct titanium_decode mmio_decode;

    /* stage-1 mmu struct(shadow) */
    struct s1mmu *s1mmu;
    bool is_vm_mmu_enable;
    bool use_shadow_ttbr0;
    int next_task_pid;
    int user_task_pid;

    unsigned long vmpidr_el2;
};

extern struct titanium_state global_titanium_states[SVISOR_PHYSICAL_CORE_NUM];

#define DEFINE(sym, val) \
    asm volatile("\n.ascii \"->" #sym " %0 " #val "\"" : : "i" (val))

#define asmoffsetof(TYPE, MEMBER) ((unsigned long)&((TYPE *)0)->MEMBER)

// extern uint64_t enter_guest(uint32_t core_id);
// extern uint64_t exit_guest(void);
// extern uint64_t el1h_sync(void);
// extern uint64_t titanium_hyp_vector(void);

#define get_guest_sys_reg(state, reg) \
    (state->current_vcpu_ctx.sys_regs.reg)

#define set_guest_sys_reg(state, reg, val) \
    do { \
        state->current_vcpu_ctx.sys_regs.reg = val; \
    } while (0)

#define get_guest_gp_reg(state, reg_num) \
    (state->current_vcpu_ctx.gp_regs.x[reg_num])

#define set_guest_gp_reg(state, reg_num, val) \
    do { \
        state->current_vcpu_ctx.gp_regs.x[reg_num] = val; \
    } while (0)

#define get_host_reg(state, reg) \
    (state->host_state.reg)

/* sel1 emulate related */
// void emulate_access_trap(struct titanium_state *state, uint32_t core_id, uint32_t vcpu_id);
// void emulate_sys_reg_start(struct titanium_state *state, unsigned long Rt, unsigned long Rt_val);
// void emulate_sys_reg_finish(struct titanium_state *state);
// int emulate_mmio_fault(struct titanium_state *state, unsigned long fault_ipa, uint64_t vcpu_id);

#endif
