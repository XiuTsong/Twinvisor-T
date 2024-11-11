/*
 * @Description: titanium list.h
 * @Date: 2024-11-08 16:35:21
 */

#ifndef __SVISOR_LIST_H__
#define __SVISOR_LIST_H__

#include <linux/types.h>

static inline void list_init(struct list_head* head)
{
    head->next = head;
    head->prev = head;
}

static inline int list_empty(struct list_head* head)
{
    return (head->next == head);
}

static inline void list_remove(struct list_head *node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->next = NULL;
    node->prev = NULL;
}

static inline void list_push(struct list_head* head, struct list_head *node)
{
    node->next = head->next;
    node->prev = head;
    node->next->prev = node;
    node->prev->next = node;
}

static inline void list_append(struct list_head* head, struct list_head *node)
{
    struct list_head *tail = head->prev;
    list_push(tail, node);
}

static inline struct list_head *list_pop(struct list_head* head)
{
    struct list_head *node = NULL;

    if(head->next == head){
        return NULL;
    }
    node = head->next;
    list_remove(node);
    return node;
}

#endif