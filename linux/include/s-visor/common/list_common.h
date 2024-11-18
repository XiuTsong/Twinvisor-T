/*
 * @Date: 2024-11-18 14:53:48
 */

#ifndef __SVISOR_COMMON_LIST_H__
#define __SVISOR_COMMON_LIST_H__

struct list_head {
	struct list_head *next, *prev;
};

#endif
