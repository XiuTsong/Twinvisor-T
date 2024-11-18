#include <asm/asm-offsets.h>

#include <s-visor/common/asm.h>
#include <s-visor/lib/el3_runtime/smc.h>
#include <s-visor/el3/titanium_private.h>

.macro save_args_to_vm ctxt
	stp	x0, x1,   [\ctxt, #CPU_XREG_OFFSET(0)]
	stp	x2, x3,   [\ctxt, #CPU_XREG_OFFSET(2)]
	stp	x4, x5,   [\ctxt, #CPU_XREG_OFFSET(4)]
	stp	x6, x7,   [\ctxt, #CPU_XREG_OFFSET(6)]
.endm

.macro restore_args_from_vm ctxt
	ldp	x0, x1,   [\ctxt, #CPU_XREG_OFFSET(0)]
	ldp	x2, x3,   [\ctxt, #CPU_XREG_OFFSET(2)]
	ldp	x4, x5,   [\ctxt, #CPU_XREG_OFFSET(4)]
	ldp	x6, x7,   [\ctxt, #CPU_XREG_OFFSET(6)]
.endm

	.pushsection ".svisor.text", "awx"
LOCAL_TITANIUM_ENTRY(vector_std_smc_entry)
	b	.	/* SMC should not return */
TITANIUM_ENTRY_END(vector_std_smc_entry)

LOCAL_TITANIUM_ENTRY(vector_fast_smc_entry)
	b	.	/* SMC should not return */
TITANIUM_ENTRY_END(vector_fast_smc_entry)

LOCAL_TITANIUM_ENTRY(vector_kvm_trap_smc_entry)
        b	.	/* SMC should not return */
TITANIUM_ENTRY_END(vector_kvm_trap_smc_entry)

/*
 * x1 - physical address of shared memory base
 */
LOCAL_TITANIUM_ENTRY(vector_kvm_shared_memory_register_entry)
	ldr x0, =titanium_shared_pages_phys
	str x1, [x0]
	bl	kvm_shared_memory_register
	__smc #SMC_IMM_TITANIUM_TO_KVM_SHARED_MEMORY
	b	.	/* SMC should not return */
TITANIUM_ENTRY_END(vector_kvm_shared_memory_register_entry)

LOCAL_TITANIUM_ENTRY(vector_kvm_shared_memory_handle_entry)
	bl	kvm_shared_memory_handle
	__smc #SMC_IMM_TITANIUM_TO_KVM_SHARED_MEMORY
    b	.	/* SMC should not return */
TITANIUM_ENTRY_END(vector_kvm_shared_memory_handle_entry)

LOCAL_TITANIUM_ENTRY(vector_cpu_on_entry)
        b  .        /* SMC should not return */
TITANIUM_ENTRY_END(vector_cpu_on_entry)

LOCAL_TITANIUM_ENTRY(vector_cpu_off_entry)
        b  . 	/* SMC should not return */
TITANIUM_ENTRY_END(vector_cpu_off_entry)

LOCAL_TITANIUM_ENTRY(vector_cpu_resume_entry)
        b  . 	/* SMC should not return */
TITANIUM_ENTRY_END(vector_cpu_resume_entry)

LOCAL_TITANIUM_ENTRY(vector_cpu_suspend_entry)
        b  . 	/* SMC should not return */
TITANIUM_ENTRY_END(vector_cpu_suspend_entry)

LOCAL_TITANIUM_ENTRY(vector_fiq_entry)
        b  . 	/* SMC should not return */
TITANIUM_ENTRY_END(vector_fiq_entry)

LOCAL_TITANIUM_ENTRY(vector_system_off_entry)
        b  . 	/* SMC should not return */
TITANIUM_ENTRY_END(vector_system_off_entry)

LOCAL_TITANIUM_ENTRY(vector_system_reset_entry)
        b  . 	/* SMC should not return */
TITANIUM_ENTRY_END(vector_system_reset_entry)

.globl	init_primary_core
.globl	init_secondary_core
.globl	titanium_hyp_vector
.globl	__svisor_early_stack

/*
 * x1 - vp_offset,
 */
LOCAL_TITANIUM_ENTRY(vector_primary_core_entry)
	ldr x2 , =linux_vp_offset
	str x1, [x2]
	msr spsel, #1
	ldr x0, =__svisor_early_stack
	ldr x1, =0x1000
	add x0, x0, x1
	mov sp, x0

	ldr x1, =global_titanium_states
	mov x0, #0 // primary core id is 0
	mov w2, #GLOBAL_TITANIUM_STATE_SIZE
	umaddl x3, w0, w2, x1 // x3 = x1 + w0 * w2
	msr tpidr_el1, x3
	// store tpidr_el1 in global entry
	ldr x1, =g_tpidr
	mov w2, #8
	umaddl x1, w0, w2, x1
	str x3, [x1]
	// install fault handlers
	ldr x0, =titanium_hyp_vector
	msr vbar_el1,x0
	bl  init_primary_core
	// set guest sys-regs the same as svisor
	bl  set_guest_sys_regs
	mrs     x9, tpidr_el1
	add     x9, x9, #VCPU_CTX_OFFSET
	add     x9, x9, #GUEST_GP_REGS_OFFSET
	ldp	    x0, x1, [x9, #CPU_XREG_OFFSET(0)]
	ldp	    x2, x3, [x9, #CPU_XREG_OFFSET(2)]
	ldp	    x4, x5, [x9, #CPU_XREG_OFFSET(4)]
	ldp	    x6, x7, [x9, #CPU_XREG_OFFSET(6)]

	__smc #SMC_IMM_TITANIUM_TO_KVM_RETURN
TITANIUM_ENTRY_END(vector_primary_core_entry)

/* In Twinvisor emulation, we use this entry instead of vector_cpu_on_entry */
LOCAL_TITANIUM_ENTRY(vector_secondary_core_entry)
	/* Prepare stack */
	bl	__get_core_pos
	ldr	x1, =current_cpu_stack_sps
	mov	w2, #8
	umaddl	x1, w0, w2, x1  // x0 = x1 + w0 * w2
	ldr	x1, [x1]
	mov	sp, x1

	/* Prepare global_titanium_states and g_tpidr */
	ldr x1, =global_titanium_states
	mov w2, #GLOBAL_TITANIUM_STATE_SIZE
	umaddl x3, w0, w2, x1  // x3 = x1 + w0 * w2
	msr tpidr_el1, x3
	ldr x1, =g_tpidr
	mov w2, #8
	umaddl x1, w0, w2, x1
	str x3, [x1]

	/* install fault handlers */
	ldr x0, =titanium_hyp_vector
	msr vbar_el1,x0

	bl init_secondary_core

	__smc #SMC_IMM_TITANIUM_TO_KVM_RETURN
TITANIUM_ENTRY_END(vector_secondary_core_entry)

	.popsection

	.pushsection ".svisor_handler.text", "ax"

TITANIUM_ENTRY(titanium_handler_table)
	b	vector_std_smc_entry
	b	vector_fast_smc_entry
	b	vector_kvm_trap_smc_entry
	b	vector_kvm_shared_memory_register_entry
	b	vector_kvm_shared_memory_handle_entry
	b	vector_cpu_on_entry
	b	vector_cpu_off_entry
	b	vector_cpu_resume_entry
	b	vector_cpu_suspend_entry
	b	vector_fiq_entry
	b	vector_system_off_entry
	b	vector_system_reset_entry
	b	vector_primary_core_entry
	b	vector_secondary_core_entry
TITANIUM_ENTRY_END(titanium_handler_table)

	.popsection