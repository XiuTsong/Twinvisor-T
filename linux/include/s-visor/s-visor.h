/*
 * Created on 2024/11/05
 */

#ifndef __S_VISOR_H__
#define __S_VISOR_H__

#include <linux/string.h>

#include <s-visor/common_defs.h>
#include <s-visor/lib/stdio.h>

#define __secure_text	__section(.svisor.text)
#define __secure_data	__section(.svisor.data)

#ifdef CONFIG_S_VISOR_DEBUG
#define SVISOR_DEBUG
#endif

#endif
