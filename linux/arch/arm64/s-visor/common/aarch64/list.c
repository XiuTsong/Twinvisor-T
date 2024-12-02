/*
 * @Date: 2024-11-18 15:05:22
 */

#include <s-visor/common/list.h>
#include <s-visor/s-visor.h>

#include <linux/stddef.h>
#include <linux/kernel.h>

void __secure_text list_init(struct list_head* head)
{
	head->next = head;
	head->prev = head;
}

void __secure_text list_remove(struct list_head *node)
{
	node->next->prev = node->prev;
	node->prev->next = node->next;
	node->next = NULL;
	node->prev = NULL;
}

void __secure_text list_push(struct list_head* head, struct list_head *node)
{
	node->next = head->next;
	node->prev = head;
	node->next->prev = node;
	node->prev->next = node;
}

void __secure_text list_append(struct list_head* head, struct list_head *node)
{
	struct list_head *tail = head->prev;
	list_push(tail, node);
}

struct list_head * __secure_text list_pop(struct list_head* head)
{
	struct list_head *node = NULL;

	if(head->next == head){
		return NULL;
	}
	node = head->next;
	list_remove(node);
	return node;
}
