#include <s-visor/common/asm.h>
#include <s-visor/arch/arm64/arch.h>
#include <s-visor/arch/arm64/misc.h>


	.pushsection ".svisor.text", "ax"
/* uint64_t __get_core_pos(void); */
BEGIN_FUNC(__get_core_pos)
	__get_core_id x0, x3, x1
	ret
END_FUNC(__get_core_pos)
	.popsection
