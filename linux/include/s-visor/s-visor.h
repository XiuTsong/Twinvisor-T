/*
 * Created on 2024/11/05
 */

#ifndef __S_VISOR_H__
#define __S_VISOR_H__

#include <linux/printk.h>
#include <linux/string.h>

#include <s-visor/common_defs.h>

#define __secure_text	__section(.svisor.text)
#define __secure_data	__section(.svisor.data)

#define printf(fmt, ...)

#define SVISOR_DEBUG

#endif