/*
 * @Description: trusted-firmware-a context_mgmt.h
 * @Date: 2024-11-12 18:21:19
 */
#include <linux/types.h>
#include <s-visor/lib/stdint.h>
#include <s-visor/lib/el3_runtime/context.h>

/*******************************************************************************
 * Function & variable prototypes
 ******************************************************************************/
void *cm_get_context_by_index(uint32_t cpu_idx, uint32_t security_state);
void cm_set_context_by_index(uint32_t cpu_idx, void *context, uint32_t security_state);
void *cm_get_context(uint32_t security_state);
void cm_set_context(void *context, uint32_t security_state);

void cm_el1_sysregs_context_save(uint32_t security_state);
void cm_el1_sysregs_context_restore(uint32_t security_state);

void cm_el2_sysregs_context_save(uint32_t security_state, uint32_t is_host_only);
void cm_el2_sysregs_context_restore(uint32_t security_state, uint32_t is_host_only);

void cm_set_elr_el3(uint32_t security_state, uintptr_t entrypoint);
uint64_t cm_get_elr_el3(uint32_t security_state);
uint64_t cm_get_vbar_el2(uint32_t security_state);
void cm_set_elr_spsr_el3(uint32_t security_state,
			uintptr_t entrypoint, uint32_t spsr);
void cm_set_next_eret_context(uint32_t security_state);

/*******************************************************************************
 * This function is used to program the context that's used for exception
 * return. This initializes the SP_EL3 to a pointer to a 'cpu_context' set for
 * the required security state
 ******************************************************************************/
extern void cm_set_next_context(void *context);
extern cpu_context_t* cm_get_next_context(uint32_t security_state);
