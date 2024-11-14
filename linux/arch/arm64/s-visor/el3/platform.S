/*
 * Date: 2024-11-12 18:06:30
 */

#include <s-visor/common/asm.h>
#include <s-visor/arch/arm64/arch.h>
#include <s-visor/arch/arm64/misc.h>

	.pushsection ".el3.text", "ax"
/* uint64_t __get_core_pos(void); */
BEGIN_FUNC(plat_my_core_pos)
	__get_core_id x0, x3, x1
	ret
END_FUNC(plat_my_core_pos)
	.popsection
