#ifndef _LINUX_LIST_H
#define _LINUX_LIST_H


//#include <linux/prefetch.h>
static inline void prefetch(const void* x)
{
    ;
}

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct list_head
{
    struct list_head* next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
        (ptr)->next = (ptr); (ptr)->prev = (ptr); \
    } while (0)

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head* newp,
                              struct list_head* prev,
                              struct list_head* next)
{
    if (NULL == next || NULL == prev || NULL == newp)
    {
        return;
    }
    
    next->prev = newp;
    newp->next = next;
    newp->prev = prev;
    prev->next = newp;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void llist_add(struct list_head* newp, struct list_head* head)
{
    if (NULL == newp || NULL == head)
    {
        return;
    }

    __list_add(newp, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void llist_add_tail(struct list_head* newp, struct list_head* head)
{
    if (NULL == newp || NULL == head)
    {
        return;
    }

    __list_add(newp, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head* prev, struct list_head* next)
{
    if (NULL == next || NULL == prev)
    {
        return;
    }

    next->prev = prev;
    prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is in an undefined state.
 */
static inline void llist_del(struct list_head* entry)
{
    if (NULL == entry)
    {
        return;
    }

    __list_del(entry->prev, entry->next);
    //entry->next = (void *) 0;
    //entry->prev = (void *) 0;
    entry->next = (list_head*) 0;
    entry->prev = (list_head*) 0;
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void llist_del_init(struct list_head* entry)
{
    if (NULL == entry)
    {
        return;
    }

    __list_del(entry->prev, entry->next);
    INIT_LIST_HEAD(entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void llist_move(struct list_head* _list, struct list_head* head)
{
    if (NULL == _list || NULL == head)
    {
        return;
    }

    __list_del(_list->prev, _list->next);
    llist_add(_list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void llist_move_tail(struct list_head* _list,
                                   struct list_head* head)
{
    if (NULL == _list || NULL == head)
    {
        return;
    }

    __list_del(_list->prev, _list->next);
    llist_add_tail(_list, head);
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int llist_empty(struct list_head* head)
{
    if (NULL == head)
    {
        return -1;
    }

    return head->next == head;
}

static inline void __list_splice(struct list_head* _list,
                                 struct list_head* head)
{
    if (NULL == _list || NULL == head)
    {
        return;
    }

    struct list_head* first = _list->next;
    struct list_head* last = _list->prev;
    struct list_head* at = head->next;

    first->prev = head;
    head->next = first;

    last->next = at;
    at->prev = last;
}

/**
 * list_splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void llist_splice(struct list_head* _list, struct list_head* head)
{
    if (NULL == _list || NULL == head)
    {
        return;
    }

    if (!llist_empty(_list))
    {
        __list_splice(_list, head);
    }
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void llist_splice_init(struct list_head* _list,
                                     struct list_head* head)
{
    if (NULL == _list || NULL == head)
    {
        return;
    }

    if (!llist_empty(_list))
    {
        __list_splice(_list, head);
        INIT_LIST_HEAD(_list);
    }
}

/**
 * list_entry - get the struct for this entry
 * @ptr:    the &struct list_head pointer.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the list_struct within the struct.
 */
#define llist_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * list_for_each    -   iterate over a list
 * @pos:    the &struct list_head to use as a loop counter.
 * @head:   the head for your list.
 */
#define llist_for_each(pos, head) \
    for (pos = (head)->next, prefetch(pos->next); pos != (head); \
            pos = pos->next, prefetch(pos->next))
/**
 * list_for_each_prev   -   iterate over a list backwards
 * @pos:    the &struct list_head to use as a loop counter.
 * @head:   the head for your list.
 */
#define llist_for_each_prev(pos, head) \
    for (pos = (head)->prev, prefetch(pos->prev); pos != (head); \
            pos = pos->prev, prefetch(pos->prev))

/**
 * list_for_each_safe   -   iterate over a list safe against removal of list entry
 * @pos:    the &struct list_head to use as a loop counter.
 * @n:      another &struct list_head to use as temporary storage
 * @head:   the head for your list.
 */
#define llist_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
            pos = n, n = pos->next)

/**
 * list_for_each_entry  -   iterate over list of given type
 * @pos:    the type * to use as a loop counter.
 * @head:   the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define llist_for_each_entry(pos, head, member)             \
    for (pos = llist_entry((head)->next, typeof(*pos), member), \
            prefetch(pos->member.next);            \
            &pos->member != (head);                    \
            pos = llist_entry(pos->member.next, typeof(*pos), member), \
            prefetch(pos->member.next))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:    the type * to use as a loop counter.
 * @n:      another type * to use as temporary storage
 * @head:   the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define llist_for_each_entry_safe(pos, n, head, member)         \
    for (pos = llist_entry((head)->next, typeof(*pos), member), \
            n = llist_entry(pos->member.next, typeof(*pos), member);    \
            &pos->member != (head);                    \
            pos = n, n = llist_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_continue -       iterate over list of given type
 *                      continuing after existing point
 * @pos:        the type * to use as a loop counter.
 * @head:       the head for your list.
 * @member:     the name of the list_struct within the struct.
 */
#define llist_for_each_entry_continue(pos, head, member)            \
    for (pos = llist_entry(pos->member.next, typeof(*pos), member), \
            prefetch(pos->member.next);            \
            &pos->member != (head);                    \
            pos = llist_entry(pos->member.next, typeof(*pos), member), \
            prefetch(pos->member.next))


#endif
