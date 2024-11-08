#include <asm/asm-offsets.h>

#include <s-visor/common/asm.h>
#include <s-visor/virt/registers-asm.h>

/*
 * Store svisor's sys-regs in guest's context.
 */
BEGIN_FUNC(set_guest_sys_regs)
    stp x0, x1, [sp, #-16]!
    get_guest_sys_regs  x1
    mrs x0, sctlr_el1
    str	x0, [x1, #SYS_SCTLR_OFFSET]
    mrs x0, mair_el1
    str	x0, [x1, #SYS_MAIR_OFFSET]
    mrs x0, amair_el1
    str	x0, [x1, #SYS_AMAIR_OFFSET]
    mrs x0, tcr_el1
    str	x0, [x1, #SYS_TCR_OFFSET]
    ldp x0, x1, [sp], #16
    ret
END_FUNC(set_guest_sys_regs)

BEGIN_FUNC(set_guest_sctlr)
    str x1, [sp, #-8]!
    get_guest_sys_regs  x1
    str	x0, [x1, #SYS_SCTLR_OFFSET]
    ldr x1, [sp], #8
    ret
END_FUNC(set_guest_sctlr)

BEGIN_FUNC(get_guest_spsr)
    get_guest_sys_regs x0
    ldr	        x0, [x0, #SYS_SPSR_OFFSET]
    ret
END_FUNC(get_guest_spsr)

BEGIN_FUNC(set_guest_tpidr)
    str x1, [sp, #-8]!
    get_guest_sys_regs  x1
    str	x0, [x1, #SYS_TPIDR_OFFSET]
    ldr x1, [sp], #8
    ret
END_FUNC(set_guest_tpidr)
