/*
 *Description: context management related macro header file
 *Date: 2024-11-25 16:41:13
 */

#ifndef __SVISOR_CONTEXT_MACROS_S__
#define __SVISOR_CONTEXT_MACROS_S__

#include <asm/asm-offsets.h>

#include <s-visor/virt/registers-asm.h>
#include <s-visor/arch/arm64/misc.h>
#include <s-visor/lib/el3_runtime/context.h>
#include <s-visor/el3/asm_macros.S>
#include <s-visor/el3/el3_common_macros.S>

.macro cm_get_context_secure
	__get_core_id x1, x2, x3 /* x1 = core_id */
	ldr x2, =titanium_secure_context
	mov x3, #TITANIUM_CONTEXT_SIZE
	umaddl x0, w1, w3, x2 /* x0 = &titanium_(non_)secure_context[core_id] */
	add x0, x0, #CPU_CONTEXT_OFFSET
.endm

.macro cm_get_context_ns
	__get_core_id x1, x2, x3 /* x1 = core_id */
	ldr x2, =titanium_non_secure_context
	mov x3, #TITANIUM_CONTEXT_SIZE
	umaddl x0, w1, w3, x2 /* x0 = &titanium_(non_)secure_context[core_id] */
	add x0, x0, #CPU_CONTEXT_OFFSET
.endm

.macro cm_get_gp_regs_secure
	cm_get_context_secure
	add x0, x0, #CTX_GPREGS_OFFSET
.endm

.macro cm_save_gp_ctx_secure
	stp x0, x1, [sp, #-16]!
	stp x2, x3, [sp, #-16]!
	cm_get_gp_regs_secure /* Use x0, x1, x2, x3; Return x0 */
	ldp x2, x3, [sp], #16
	save_all_gp_regs x0 /* Save x2-x30 */
	ldp x2, x3, [sp], #16 /* Restore x0, x1 in x2, x3 */
	stp x2, x3, [x0, #CPU_XREG_OFFSET(0)] /* Save the value of x0, x1 */
.endm

.macro cm_restore_gp_ctx_secure
	cm_get_gp_regs_secure /* x0 */
	restore_all_gp_regs x0 /* Restore x2-x30 */
	ldp x0, x1, [x0, #CPU_XREG_OFFSET(0)] /* Restore of x0, x1 */
.endm

#endif
