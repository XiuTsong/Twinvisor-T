/*
 * @Description: trusted-firmware-a runtime_svc.h
 * @Date: 2024-11-11 19:19:00
 */

#ifndef __SVISOR_EL3_RUNTIME_SVC_H__
#define __SVISOR_EL3_RUNTIME_SVC_H__

#include <s-visor/el3/section.h>
#include <s-visor/lib/utils_def.h>

/*
 * Constants to allow the assembler access a runtime service
 * descriptor
 */
#ifdef __aarch64__
#define RT_SVC_SIZE_LOG2	U(5)
#define RT_SVC_DESC_INIT	U(16)
#define RT_SVC_DESC_HANDLE	U(24)
#else
#define RT_SVC_SIZE_LOG2	U(4)
#define RT_SVC_DESC_INIT	U(8)
#define RT_SVC_DESC_HANDLE	U(12)
#endif /* __aarch64__ */
#define SIZEOF_RT_SVC_DESC	(U(1) << RT_SVC_SIZE_LOG2)

/*
 * In SMCCC 1.X, the function identifier has 6 bits for the owning entity number
 * and a single bit for the type of smc call. When taken together, those values
 * limit the maximum number of runtime services to 128.
 */
#define MAX_RT_SVCS		U(128)

#ifndef __ASSEMBLER__

#include <s-visor/lib/stdint.h>
#include <s-visor/el3/section.h>
#include <s-visor/lib/el3_runtime/smccc.h>

typedef int32_t (*rt_svc_init_t)(void);

typedef uintptr_t (*rt_svc_handle_t)(uint32_t smc_fid,
				  u_register_t x1,
				  u_register_t x2,
				  u_register_t x3,
				  u_register_t x4,
				  void *cookie,
				  void *handle,
				  u_register_t flags);

typedef struct rt_svc_desc {
	uint8_t start_oen;
	uint8_t end_oen;
	uint8_t call_type;
	const char *name;
	rt_svc_init_t init;
	rt_svc_handle_t handle;
} rt_svc_desc_t;

/*
 * Convenience macros to declare a service descriptor
 */
#define DECLARE_RT_SVC(_name, _start, _end, _type, _setup, _smch)	\
	static const rt_svc_desc_t __svc_desc_ ## _name			\
		__el3_rt_svc_desc __used = {			\
			.start_oen = (_start),				\
			.end_oen = (_end),				\
			.call_type = (_type),				\
			.name = #_name,					\
			.init = (_setup),				\
			.handle = (_smch)				\
		}

/*
 * This function combines the call type and the owning entity number
 * corresponding to a runtime service to generate a unique owning entity number.
 * This unique oen is used to access an entry in the 'rt_svc_descs_indices'
 * array. The entry contains the index of the service descriptor in the
 * 'rt_svc_descs' array.
 */
static inline uint32_t get_unique_oen(uint32_t oen, uint32_t call_type)
{
	return ((call_type & FUNCID_TYPE_MASK) << FUNCID_OEN_WIDTH) |
		(oen & FUNCID_OEN_MASK);
}

/*
 * This function generates the unique owning entity number from the SMC Function
 * ID. This unique oen is used to access an entry in the 'rt_svc_descs_indices'
 * array to invoke the corresponding runtime service handler during SMC
 * handling.
 */
static inline uint32_t get_unique_oen_from_smc_fid(uint32_t fid)
{
	return get_unique_oen(GET_SMC_OEN(fid), GET_SMC_TYPE(fid));
}

/*******************************************************************************
 * Function & variable prototypes
 ******************************************************************************/
void runtime_svc_init(void);

extern uint8_t rt_svc_descs_indices[MAX_RT_SVCS];

#define RT_SVC_DESCS_START __RT_SVC_DESCS_START__
#define RT_SVC_DESCS_END __RT_SVC_DESCS_END__

#endif

#endif
