/*
 * @Description: trusted-firmware-a runtime_svc.c
 * @Date: 2024-11-11 19:42:34
 */

#include <linux/string.h>
#include <linux/errno.h>

#include <s-visor/s-visor.h>
#include <s-visor/el3/runtime_svc.h>
#include <s-visor/el3/section.h>
#include <s-visor/lib/el3_runtime/smccc.h>

#ifdef SVISOR_DEBUG
#pragma GCC optimize("O0")
#endif

/*******************************************************************************
 * The 'rt_svc_descs' array holds the runtime service descriptors exported by
 * services by placing them in the 'rt_svc_descs' linker section.
 * The 'rt_svc_descs_indices' array holds the index of a descriptor in the
 * 'rt_svc_descs' array. When an SMC arrives, the OEN[29:24] bits and the call
 * type[31] bit in the function id are combined to get an index into the
 * 'rt_svc_descs_indices' array. This gives the index of the descriptor in the
 * 'rt_svc_descs' array which contains the SMC handler.
 ******************************************************************************/
uint8_t __el3_data rt_svc_descs_indices[MAX_RT_SVCS] = { [0 ... (MAX_RT_SVCS-1)] = 0 };

#define RT_SVC_DECS_NUM		((RT_SVC_DESCS_END - RT_SVC_DESCS_START)\
					/ sizeof(rt_svc_desc_t))

/*******************************************************************************
 * Simple routine to sanity check a runtime service descriptor before using it
 ******************************************************************************/
static int32_t __el3_text validate_rt_svc_desc(const rt_svc_desc_t *desc)
{
	if (desc == NULL)
		return -EINVAL;

	if (desc->start_oen > desc->end_oen)
		return -EINVAL;

	if (desc->end_oen >= OEN_LIMIT)
		return -EINVAL;

	if ((desc->call_type != SMC_TYPE_FAST) &&
	    (desc->call_type != SMC_TYPE_YIELD))
		return -EINVAL;

	/* A runtime service having no init or handle function doesn't make sense */
	if ((desc->init == NULL) && (desc->handle == NULL))
		return -EINVAL;

	return 0;
}

/*******************************************************************************
 * This function calls the initialisation routine in the descriptor exported by
 * a runtime service. Once a descriptor has been validated, its start & end
 * owning entity numbers and the call type are combined to form a unique oen.
 * The unique oen is used as an index into the 'rt_svc_descs_indices' array.
 * The index of the runtime service descriptor is stored at this index.
 ******************************************************************************/
void __el3_text runtime_svc_init(void)
{
	int rc = 0;
	uint8_t index, start_idx, end_idx;
	rt_svc_desc_t *rt_svc_descs;

	/* Initialise internal variables to invalid state */
	(void)memset(rt_svc_descs_indices, -1, sizeof(rt_svc_descs_indices));

	rt_svc_descs = (rt_svc_desc_t *)RT_SVC_DESCS_START;
	for (index = 0U; index < RT_SVC_DECS_NUM; index++) {
		rt_svc_desc_t *service = &rt_svc_descs[index];

		/*
		 * An invalid descriptor is an error condition since it is
		 * difficult to predict the system behaviour in the absence
		 * of this service.
		 */
		rc = validate_rt_svc_desc(service);
		if (rc != 0) {
			// panic();
            asm volatile("b .\n");
		}

		/*
		 * The runtime service may have separate rt_svc_desc_t
		 * for its fast smc and yielding smc. Since the service itself
		 * need to be initialized only once, only one of them will have
		 * an initialisation routine defined. Call the initialisation
		 * routine for this runtime service, if it is defined.
		 */
		if (service->init != NULL) {
			rc = service->init();
			if (rc != 0) {
				// ERROR("Error initializing runtime service %s\n",
				// 		service->name);
				continue;
			}
		}

		/*
		 * Fill the indices corresponding to the start and end
		 * owning entity numbers with the index of the
		 * descriptor which will handle the SMCs for this owning
		 * entity range.
		 */
		start_idx = (uint8_t)get_unique_oen(service->start_oen,
						    service->call_type);
		end_idx = (uint8_t)get_unique_oen(service->end_oen,
						  service->call_type);
		for (; start_idx <= end_idx; start_idx++)
			rt_svc_descs_indices[start_idx] = index;
	}
}
