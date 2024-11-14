/*
 * @Description: trusted-firmware-a titanium_private.h
 * @Date: 2024-11-12 18:44:03
 */

#ifndef __SVISOR_TITANIUM_PRIVATE_H__
#define __SVISOR_TITANIUM_PRIVATE_H__

#include <s-visor/common_defs.h>

/*******************************************************************************
 * TITANIUM PM state information e.g. TITANIUM is suspended, uninitialised etc
 * and macros to access the state information in the per-cpu 'state' flags
 ******************************************************************************/
#define TITANIUM_PSTATE_OFF		0
#define TITANIUM_PSTATE_ON			1
#define TITANIUM_PSTATE_SUSPEND		2
#define TITANIUM_PSTATE_SHIFT		0
#define TITANIUM_PSTATE_MASK		0x3
#define get_titanium_pstate(state)	((state >> TITANIUM_PSTATE_SHIFT) & \
				 TITANIUM_PSTATE_MASK)
#define clr_titanium_pstate(state)	(state &= ~(TITANIUM_PSTATE_MASK \
					    << TITANIUM_PSTATE_SHIFT))
#define set_titanium_pstate(st, pst) do {					       \
					clr_titanium_pstate(st);		       \
					st |= (pst & TITANIUM_PSTATE_MASK) <<     \
						TITANIUM_PSTATE_SHIFT;	       \
				} while (0)


/*******************************************************************************
 * TITANIUM execution state information i.e. aarch32 or aarch64
 ******************************************************************************/
#define TITANIUM_AARCH32		MODE_RW_32
#define TITANIUM_AARCH64		MODE_RW_64

// /*******************************************************************************
//  * The TITANIUM should know the type of TITANIUM
//  ******************************************************************************/
// #define TITANIUM_TYPE_UP		PSCI_TOS_NOT_UP_MIG_CAP
// #define TITANIUM_TYPE_UPM		PSCI_TOS_UP_MIG_CAP
// #define TITANIUM_TYPE_MP		PSCI_TOS_NOT_PRESENT_MP

// /*******************************************************************************
//  * TITANIUM migrate type information as known to the TITANIUM. We assume that
//  * the TITANIUM is dealing with an MP Secure Payload.
//  ******************************************************************************/
// #define TITANIUM_MIGRATE_INFO		TITANIUM_TYPE_MP

/*******************************************************************************
 * Number of cpus that the present on this platform. TODO: Rely on a topology
 * tree to determine this in the future to avoid assumptions about mpidr
 * allocation
 ******************************************************************************/
#define TITANIUM_CORE_COUNT		SVISOR_PHYSICAL_CORE_NUM

/*******************************************************************************
 * Constants that allow assembler code to preserve callee-saved registers of the
 * C runtime context while performing a security state switch.
 ******************************************************************************/
#define TITANIUM_C_RT_CTX_X19		0x0
#define TITANIUM_C_RT_CTX_X20		0x8
#define TITANIUM_C_RT_CTX_X21		0x10
#define TITANIUM_C_RT_CTX_X22		0x18
#define TITANIUM_C_RT_CTX_X23		0x20
#define TITANIUM_C_RT_CTX_X24		0x28
#define TITANIUM_C_RT_CTX_X25		0x30
#define TITANIUM_C_RT_CTX_X26		0x38
#define TITANIUM_C_RT_CTX_X27		0x40
#define TITANIUM_C_RT_CTX_X28		0x48
#define TITANIUM_C_RT_CTX_X29		0x50
#define TITANIUM_C_RT_CTX_X30		0x58
#define TITANIUM_C_RT_CTX_SIZE		0x60
#define TITANIUM_C_RT_CTX_ENTRIES		(TITANIUM_C_RT_CTX_SIZE >> DWORD_SHIFT)

#define SMC_IMM_KVM_TO_TITANIUM_TRAP 0x1
#define SMC_IMM_TITANIUM_TO_KVM_TRAP_SYNC 0x1
#define SMC_IMM_TITANIUM_TO_KVM_TRAP_IRQ 0x2
#define SMC_IMM_KVM_TO_TITANIUM_SHARED_MEMORY_REGISTER 0x10
#define SMC_IMM_KVM_TO_TITANIUM_SHARED_MEMORY_HANDLE 0x18
#define SMC_IMM_TITANIUM_TO_KVM_SHARED_MEMORY 0x10
#define SMC_IMM_TITANIUM_TO_KVM_FIXUP_VTTBR 0x20
#define SMC_IMM_KVM_TO_TITANIUM_PRIMARY 		0x100
#define SMC_IMM_KVM_TO_TITANIUM_SECONDARY 		0x101
/* Return to where n-visor call "smc" */
#define SMC_IMM_TITANIUM_TO_KVM_RETURN			0x102

#ifndef __ASSEMBLER__

#include <s-visor/lib/stdint.h>
#include <s-visor/lib/cassert.h>
#include <s-visor/lib/el3_runtime/context.h>

typedef uint32_t titanium_vector_isn_t;

typedef struct titanium_vectors {
	titanium_vector_isn_t yield_smc_entry;
	titanium_vector_isn_t fast_smc_entry;
	titanium_vector_isn_t kvm_trap_smc_entry;
	titanium_vector_isn_t kvm_shared_memory_register_entry;
	titanium_vector_isn_t kvm_shared_memory_handle_entry;
	titanium_vector_isn_t cpu_on_entry;
	titanium_vector_isn_t cpu_off_entry;
	titanium_vector_isn_t cpu_resume_entry;
	titanium_vector_isn_t cpu_suspend_entry;
	titanium_vector_isn_t fiq_entry;
	titanium_vector_isn_t system_off_entry;
	titanium_vector_isn_t system_reset_entry;
    titanium_vector_isn_t primary_core_entry;
    titanium_vector_isn_t secondary_core_entry;
} titanium_vectors_t;

/*
 * The number of arguments to save during a SMC call for TITANIUM.
 * Currently only x1 and x2 are used by TITANIUM.
 */
#define TITANIUM_NUM_ARGS	0x2

/* AArch64 callee saved general purpose register context structure. */
DEFINE_REG_STRUCT(c_rt_regs, TITANIUM_C_RT_CTX_ENTRIES);

/*******************************************************************************
 * Structure which helps the TITANIUM to maintain the per-cpu state of TITANIUM.
 * 'state'          - collection of flags to track TITANIUM state e.g. on/off
 * 'mpidr'          - mpidr to associate a context with a cpu
 * 'c_rt_ctx'       - stack address to restore C runtime context from after
 *                    returning from a synchronous entry into TITANIUM.
 * 'cpu_ctx'        - space to maintain TITANIUM architectural state
 ******************************************************************************/
typedef struct titanium_context {
	cpu_context_t cpu_ctx;
} titanium_context_t;

#endif /* __ASSEMBLER__ */

#endif
