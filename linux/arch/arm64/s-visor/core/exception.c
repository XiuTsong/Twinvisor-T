/*
 * @Description: titanium exception.c
 * @Date: 2024-11-08 18:31:57
 */

#include <asm/sysreg.h>

#include <s-visor/common_defs.h>
#include <s-visor/virt/vmexit_def.h>
#include <s-visor/virt/vm.h>

void el1h_sync_handler(u_register_t esr_el1)
{
    unsigned long ec = ESR_EL_EC(esr_el1);
    u_register_t elr_el1 = read_sysreg(elr_el1);
    struct titanium_state *state = NULL;

    if (ec == ESR_ELx_EC_FP_ASIMD) {
        /* Just step over */
        elr_el1 += 4;
        write_sysreg(elr_el1, elr_el1);
        /* Restore ESR_EL1 to guest's */
        state = this_core_titanium_state();
        esr_el1 = get_guest_sys_reg(state, esr);
        write_sysreg(esr_el1, esr_el1);
    }
}

void exception_handler(u_register_t exception_type)
{
    u_register_t esr_el1 = read_sysreg(elr_el1);

    switch (exception_type)
    {
        case TITANIUM_HYP_SYNC:
            el1h_sync_handler(esr_el1);
        break;
        default:
            printf("Unsupported exception type: %lu\n", exception_type);
        break;
    }
}
