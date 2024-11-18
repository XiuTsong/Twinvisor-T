/*
 * @Description: titanium list.h
 * @Date: 2024-11-08 16:35:21
 */

#ifndef __SVISOR_LIST_H__
#define __SVISOR_LIST_H__

#include <s-visor/common/list_common.h>
#include <s-visor/common/macro.h>

void list_init(struct list_head* head);

int list_empty(struct list_head* head);

void list_remove(struct list_head *node);

void list_push(struct list_head* head, struct list_head *node);

void list_append(struct list_head* head, struct list_head *node);

struct list_head *list_pop(struct list_head* head);

#define for_each_in_list(elem, type, field, head) \
	for (elem = _container_of((head)->next, type, field); \
	     &((elem)->field) != (head); \
	     elem = _container_of(((elem)->field).next, type, field))

#define for_each_in_list_safe(elem, n, type, field, head) \
	for (elem = _container_of((head)->next, type, field), \
		n = _container_of(((elem)->field).next, type, field); \
	     &((elem)->field) != (head); \
	     elem = n, n = _container_of(((elem)->field).next, type, field))

#endif