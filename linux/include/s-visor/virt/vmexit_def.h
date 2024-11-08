/*
 * @Description: titanium vmexit_def.h
 * @Date: 2024-11-08 18:24:54
 */

#ifndef __SVISOR_VMEXIT_DEF_H__
#define __SVISOR_VMEXIT_DEF_H__

#include <asm/esr.h>

#define TITANIUM_VMEXIT_SYNC                0
#define TITANIUM_VMEXIT_IRQ                 1
#define TITANIUM_VMEXIT_FIQ                 2
#define TITANIUM_VMEXIT_ERR                 3
#define TITANIUM_HYP_SYNC                   4
#define TITANIUM_HYP_IRQ                    5
#define TITANIUM_HYP_FIQ                    6
#define TITANIUM_HYP_ERR                    7
#define TITANIUM_VMEXIT_USER_SYNC           8
#define TITANIUM_VMEXIT_USER_IRQ            9
#define TITANIUM_VMEXIT_USER_FIQ            10

#define ESR_EL_EC_SHIFT	    (26)
#define ESR_EL_EC_MASK		((0x3F) << ESR_EL_EC_SHIFT)
#define ESR_EL_EC(esr)		(((esr) & ESR_EL_EC_MASK) >> ESR_EL_EC_SHIFT)

#endif