#ifndef _LIST_H
#define _LIST_H

#include <stddef.h>
#include <stdbool.h>

// 1. 核心結構
struct list_head {
    struct list_head *next, *prev;
};

// 2. 初始化
#define LIST_HEAD_INIT(name) { &(name), &(name) }

static inline void INIT_LIST_HEAD(struct list_head *list) {
    list->next = list;
    list->prev = list;
}

// 3. 插入與刪除 (核心邏輯)
static inline void __list_add(struct list_head *new_node,
                            struct list_head *prev,
                            struct list_head *next) {
    next->prev = new_node;
    new_node->next = next;
    new_node->prev = prev;
    prev->next = new_node;
}

static inline void list_add(struct list_head *new_node, struct list_head *head) {
    __list_add(new_node, head, head->next);
}

static inline void list_del(struct list_head *entry) {
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
    entry->next = NULL;
    entry->prev = NULL;
}

static inline bool list_empty(struct list_head *head) {
    return head->next == head;
}

static inline void list_replace(struct list_head *old,
				struct list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

// 4. 遍歷巨集
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

// 5. 最關鍵的「彈射」魔術：list_entry
#ifndef container_of
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#endif