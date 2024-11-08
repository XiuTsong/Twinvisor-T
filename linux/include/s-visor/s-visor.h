/*
 * Created on 2024/11/05
 */

#ifndef __S_VISOR_H__
#define __S_VISOR_H__

#include <linux/printk.h>
#include <linux/string.h>

#define __secure_text	__section(.svisor.text)
#define __secure_data	__section(.svisor.data)

#define printf pr_info

#define SVISOR_PHYSICAL_CORE_NUM 4

#endif