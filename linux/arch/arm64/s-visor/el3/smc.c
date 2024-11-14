/*
 * @Date: 2024-11-13 20:12:12
 */

#include <asm/sysreg.h>
#include <linux/irqflags.h>

#include <s-visor/s-visor.h>
#include <s-visor/el3/ep_info.h>
#include <s-visor/el3/context_mgmt.h>
#include <s-visor/lib/el3_runtime/smc.h>

#ifdef SVISOR_DEBUG
#pragma GCC optimize("O0")
#endif

void __before_nvisor_smc(struct smc_local_context *ctx)
{
	ctx->vbar_el2 = read_sysreg(vbar_el2);
	ctx->sp_el3 = (unsigned long)cm_get_next_context(NON_SECURE);
	cm_el2_sysregs_context_save(NON_SECURE, true);
	local_irq_save(ctx->flags);
	write_sysreg((unsigned long)runtime_exceptions, vbar_el2);
}

void __after_nvisor_smc(struct smc_local_context *ctx)
{
	write_sysreg(ctx->vbar_el2, vbar_el2);
	local_irq_restore(ctx->flags);
}
