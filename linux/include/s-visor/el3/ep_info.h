/*
 * @Date: 2024-11-12 19:01:39
 */

#ifndef __SVISOR_EP_INFO_H__
#define __SVISOR_EP_INFO_H__

#include <s-visor/lib/utils_def.h>

#define SECURE		U(0x0)
#define NON_SECURE	U(0x1)
#define sec_state_is_valid(s) (((s) == SECURE) || ((s) == NON_SECURE))

#endif
