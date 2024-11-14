/*
 * @Description: el3 sections
 * @Date: 2024-11-11 16:39:46
 */

#ifndef __S_VISOR_EL3_SECTION_H__
#define __S_VISOR_EL3_SECTION_H__

#define __el3_text	__section(.el3.text)
#define __el3_data	__section(.el3.data)
#define __el3_rt_svc_desc __section(.el3_rt_svc_descs.data)

#ifndef __ASSEMBLER__

extern char __RT_SVC_DESCS_START__[], __RT_SVC_DESCS_END__[];

#endif

#endif
