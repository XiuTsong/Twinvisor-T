/*
 * @Description:
 * 		Secure state management.
 * 		Partially port from trusted-firmware-a context_mgmt.c
 * @Date: 2024-11-11 15:25:31
 */

#include <s-visor/s-visor.h>
#include <s-visor/common_defs.h>
#include <s-visor/lib/el3_runtime/context.h>
#include <s-visor/lib/assert.h>
#include <s-visor/lib/el3_runtime/cpu_data.h>
#include <s-visor/lib/el3_runtime/context.h>
#include <s-visor/el3/context_mgmt.h>
#include <s-visor/el3/section.h>
#include <s-visor/el3/ep_info.h>

#pragma GCC optimize("O0")

/*******************************************************************************
 * This function returns a pointer to the 'cpu_context' structure by CPU index.
 * NULL is returned if no such structure has been specified.
 ******************************************************************************/
__el3_text void *cm_get_context_by_index(uint32_t cpu_idx, uint32_t security_state)
{
	assert(security_state <= NON_SECURE);

	return get_cpu_data_by_index(cpu_idx, cpu_context[security_state]);
}

/*******************************************************************************
 * This function sets the pointer to the 'cpu_context' structure for the
 * specified security state by CPU index.
 ******************************************************************************/
void cm_set_context_by_index(uint32_t cpu_idx, void *context, uint32_t security_state)
{
	assert(security_state <= NON_SECURE);

	set_cpu_data_by_index(cpu_idx, cpu_context[security_state], context);
}

/*******************************************************************************
 * This function returns a pointer to the most recent 'cpu_context' structure
 * for the calling CPU that was set as the context for the specified security
 * state. NULL is returned if no such structure has been specified.
 ******************************************************************************/
__el3_text void *cm_get_context(uint32_t security_state)
{
	assert(security_state <= NON_SECURE);

	return get_cpu_data(cpu_context[security_state]);
}

/*******************************************************************************
 * This function sets the pointer to the current 'cpu_context' structure for the
 * specified security state for the calling CPU
 ******************************************************************************/
__el3_text void cm_set_context(void *context, uint32_t security_state)
{
	assert(security_state <= NON_SECURE);

	set_cpu_data(cpu_context[security_state], context);
}

/*******************************************************************************
 * The next four functions are used by runtime services to save and restore
 * EL1 context on the 'cpu_context' structure for the specified security
 * state.
 ******************************************************************************/

__el3_text void cm_el1_sysregs_context_save(uint32_t security_state)
{
	cpu_context_t *ctx;

	ctx = cm_get_context(security_state);
	assert(ctx != NULL);

	el1_sysregs_context_save(get_el1_sysregs_ctx(ctx));
}

__el3_text void cm_el1_sysregs_context_restore(uint32_t security_state)
{
	cpu_context_t *ctx;

	ctx = cm_get_context(security_state);
	assert(ctx != NULL);

	el1_sysregs_context_restore(get_el1_sysregs_ctx(ctx));
}

__el3_text void cm_el2_sysregs_context_save(uint32_t security_state, uint32_t is_host_only)
{
	cpu_context_t *ctx;

	ctx = cm_get_context(security_state);
	assert(ctx != NULL);

	if (is_host_only) { // only save host context
		el2_sysregs_context_save_host_only(get_el2_sysregs_ctx(ctx));
	} else { // save all context related to el2
		el2_sysregs_context_save(get_el2_sysregs_ctx(ctx));
	}

	return;
}

__el3_text void cm_el2_sysregs_context_restore(uint32_t security_state, uint32_t is_host_only)
{
	cpu_context_t *ctx;

	ctx = cm_get_context(security_state);
	assert(ctx != NULL);

	if (is_host_only) { // only save host context
		el2_sysregs_context_restore_host_only(get_el2_sysregs_ctx(ctx));
	} else { // save all context related to el2
		el2_sysregs_context_restore(get_el2_sysregs_ctx(ctx));
	}

	return;
}

__el3_text void cm_el2_eret_state_save(uint32_t security_state)
{
	cpu_context_t *ctx;
	unsigned long elr_el2, spsr_el2;

	ctx = cm_get_context(security_state);
	assert(ctx != NULL);

	elr_el2 = read_sysreg(elr_el2);
	spsr_el2 = read_sysreg(spsr_el2);
	write_ctx_reg(get_el2_sysregs_ctx(ctx), CTX_ELR_EL2, elr_el2);
	write_ctx_reg(get_el2_sysregs_ctx(ctx), CTX_SPSR_EL2, spsr_el2);

	return;
}

__el3_text void cm_el2_eret_state_restore(uint32_t security_state)
{
	cpu_context_t *ctx;
	unsigned long elr_el2, spsr_el2;

	ctx = cm_get_context(security_state);
	assert(ctx != NULL);

	elr_el2 = read_ctx_reg(get_el2_sysregs_ctx(ctx), CTX_ELR_EL2);
	spsr_el2 = read_ctx_reg(get_el2_sysregs_ctx(ctx), CTX_SPSR_EL2);

	write_sysreg(elr_el2, elr_el2);
	write_sysreg(spsr_el2, spsr_el2);

	return;
}

/*******************************************************************************
 * This function populates ELR_EL3 member of 'cpu_context' pertaining to the
 * given security state with the given entrypoint
 * Note: Here we set elr_el3 in Twinvisor emulation
 ******************************************************************************/
__el3_text void cm_set_elr_el3(uint32_t security_state, uintptr_t entrypoint)
{
	cpu_context_t *ctx;
	el3_state_t *state;

	ctx = cm_get_context(security_state);
	assert(ctx != NULL);

	/* Populate EL3 state so that ERET jumps to the correct entry */
	state = get_el3state_ctx(ctx);
	write_ctx_reg(state, CTX_ELR_EL3, entrypoint);
}

/*******************************************************************************
 * This function populates ELR_EL3 and SPSR_EL3 members of 'cpu_context'
 * pertaining to the given security state
 ******************************************************************************/
__el3_text void cm_set_elr_spsr_el3(uint32_t security_state,
			uintptr_t entrypoint, uint32_t spsr)
{
	cpu_context_t *ctx;
	el3_state_t *state;

	ctx = cm_get_context(security_state);
	assert(ctx != NULL);

	/* Populate EL3 state so that ERET jumps to the correct entry */
	state = get_el3state_ctx(ctx);
	write_ctx_reg(state, CTX_ELR_EL3, entrypoint);
	write_ctx_reg(state, CTX_SPSR_EL3, spsr);
}

/*******************************************************************************
 * This function is used to program the context that's used for exception
 * return. This initializes the SP_EL3 to a pointer to a 'cpu_context' set for
 * the required security state
 ******************************************************************************/
__el3_text void cm_set_next_eret_context(uint32_t security_state)
{
	cpu_context_t *ctx;

	ctx = cm_get_context(security_state);
	assert(ctx != NULL);

	cm_set_next_context(ctx);
}

/*******************************************************************************
 * This function retrieves vbar_el2 member of 'cpu_context' pertaining to the
 * given security state.
 ******************************************************************************/
__el3_text uint64_t cm_get_vbar_el2(uint32_t security_state)
{
	cpu_context_t *ctx;
	el2_sysregs_t *state;

	ctx = cm_get_context(security_state);
	assert(ctx != NULL);

	state = get_el2_sysregs_ctx(ctx);
	return (uint64_t)read_ctx_reg(state, CTX_VBAR_EL2);
}
