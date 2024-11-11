#include <asm/asm-offsets.h>
#include <s-visor/common/asm.h>

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
//        get_guest_gp_regs x9
//        save_args_to_vm x9
//
//        bl      __get_core_pos
//        ldr     x1, =current_cpu_stack_sps
//        mov     w2, #8
//        umaddl  x1, w0, w2, x1  // x0 = x1 + w0 * w2
//        ldr     x1, [x1]
//        mov     sp, x1
//
//        mrs     x0, tpidr_el1
//        mov     x1, #ARM_SMCCC_STD_CALL
//        bl forward_smc_to_vm
//        get_guest_gp_regs x9
//        restore_args_from_vm x9
//	smc	#0
	b	.	/* SMC should not return */
TITANIUM_ENTRY_END(vector_std_smc_entry)

LOCAL_TITANIUM_ENTRY(vector_fast_smc_entry)
//        get_guest_gp_regs x9
//        save_args_to_vm x9
//
//        bl      __get_core_pos
//        ldr     x1, =current_cpu_stack_sps
//        mov     w2, #8
//        umaddl  x1, w0, w2, x1  // x0 = x1 + w0 * w2
//        ldr     x1, [x1]
//        mov     sp, x1
//
//        mrs     x0, tpidr_el1
//        mov     x1, #ARM_SMCCC_FAST_CALL
//        bl forward_smc_to_vm
//        get_guest_gp_regs x9
//        restore_args_from_vm x9
//	smc	#0
	b	.	/* SMC should not return */
TITANIUM_ENTRY_END(vector_fast_smc_entry)

LOCAL_TITANIUM_ENTRY(vector_kvm_trap_smc_entry)
//        save_guest_states x0, x1, x2
//
//        bl      __get_core_pos
//        ldr     x1, =current_cpu_stack_sps
//        mov     w2, #8
//        umaddl  x1, w0, w2, x1  // x1 = x1 + w0 * w2
//        ldr     x1, [x1]
//        mov     sp, x1
//
//        mrs     x0, tpidr_el1
//        mov     x1, #ARM_SMCCC_KVM_TRAP_CALL
//        bl forward_smc_to_vm
//        cbnz     x0, 1f
//        // esr_el1 may be modified for simulating.
//        // Do not restore esr_el1 here.
//        restore_guest_states_except_esr x0, x1
//	smc	#1 // sync
//        1:
//        restore_guest_states x0, x1
//        smc	#2 // irq
        b	.	/* SMC should not return */
TITANIUM_ENTRY_END(vector_kvm_trap_smc_entry)

LOCAL_TITANIUM_ENTRY(vector_kvm_shared_memory_register_entry)
//    bl 	    	kvm_shared_memory_register
//    smc	    	#0x10
    b	.	/* SMC should not return */
TITANIUM_ENTRY_END(vector_kvm_shared_memory_register_entry)

LOCAL_TITANIUM_ENTRY(vector_kvm_shared_memory_handle_entry)
//    bl 	    	kvm_shared_memory_handle
//    smc	    	#0x10
    b	.	/* SMC should not return */
TITANIUM_ENTRY_END(vector_kvm_shared_memory_handle_entry)

LOCAL_TITANIUM_ENTRY(vector_cpu_on_entry)
//        bl          __get_core_pos
//        ldr         x1, =current_cpu_stack_sps
//        mov         w2, #8
//        umaddl      x1, w0, w2, x1  // x0 = x1 + w0 * w2
//        ldr         x1, [x1]
//        mov         sp, x1
//
//        bl          secondary_core_entrypoint
//
//        /* Hardcode the same arguments with OPTEE */
//        ldr         x0, =0xbe000001
//        ldr         x1, =0x0
//        ldr         x2, =0xf2440000
//        ldr         x3, =0x8000000100
//        ldr         x4, =0x1
//        ldr         x5, =00
//        ldr         x6, =0xe051820
//        ldr         x7, =0xfffffffe
//
//        smc         #0
        b  .        /* SMC should not return */
TITANIUM_ENTRY_END(vector_cpu_on_entry)

LOCAL_TITANIUM_ENTRY(vector_cpu_off_entry)
//        smc #0
        b  . 	/* SMC should not return */
TITANIUM_ENTRY_END(vector_cpu_off_entry)

LOCAL_TITANIUM_ENTRY(vector_cpu_resume_entry)
//        smc #0
        b  . 	/* SMC should not return */
TITANIUM_ENTRY_END(vector_cpu_resume_entry)

LOCAL_TITANIUM_ENTRY(vector_cpu_suspend_entry)
//        smc #0
        b  . 	/* SMC should not return */
TITANIUM_ENTRY_END(vector_cpu_suspend_entry)

LOCAL_TITANIUM_ENTRY(vector_fiq_entry)
//        smc #0
        b  . 	/* SMC should not return */
TITANIUM_ENTRY_END(vector_fiq_entry)

LOCAL_TITANIUM_ENTRY(vector_system_off_entry)
//        smc #0
        b  . 	/* SMC should not return */
TITANIUM_ENTRY_END(vector_system_off_entry)

LOCAL_TITANIUM_ENTRY(vector_system_reset_entry)
//        smc #0
        b  . 	/* SMC should not return */
TITANIUM_ENTRY_END(vector_system_reset_entry)

.globl	init_primary_core
.globl	titanium_hyp_vector
.globl	__svisor_early_stack

LOCAL_TITANIUM_ENTRY(vector_primary_core_entry)
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
	// mov x0, #TEESMC_VM_RETURN_ENTRY_DONE
	// ldr x0, =0xbe000000
	// adr x1, titanium_handler_table
	// smc #0
	hvc #0
TITANIUM_ENTRY_END(vector_primary_core_entry)

LOCAL_TITANIUM_ENTRY(vector_secondary_core_entry)
//        smc #0
        b  . 	/* SMC should not return */
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