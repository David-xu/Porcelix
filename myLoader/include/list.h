#ifndef _LIST_H_
#define _LIST_H_

#include "typedef.h"
#include "assert.h"

struct list_head{
    struct list_head *prev;
    struct list_head *next;
};

#define LIST_HEAD_INIT(name)                            \
        {.prev = &(name), .next = &(name)}

#define LIST_HEAD(name)                                 \
        struct list_head name = LIST_HEAD_INIT(name)


/*
 *      p : the pointer or the 'struct list'
 *   type : the pointer of the container
 * member : the type of the 'p' in the 'struct type'
 */
#define GET_CONTAINER(p, type, member)                       \
        ((type *)(((u32)(p)) - ((u32)&(((type *)0)->member))))

/*
 *    p : struct list_head *
 * head : struct list_head *
 */
#define LIST_WALK_THROUGH(p, head)                           \
        for ((p) = ((struct list_head *)(head))->next;      \
             (p) != (struct list_head *)(head);             \
             (p) = (p)->next)

#define LIST_WALK_THROUTH_SAVE(p, head, t)                                  \
        for ((p) = ((struct list_head *)(head))->next, (t) = (p)->next;     \
             (p) != (struct list_head *)(head);                             \
             (p) = (t), (t) = (t)->next)

/*
 *  entry : owner struct pointer
 *   head : list_head pointer
 * member : owner.member
 */
#define LIST_FOREACH_ELEMENT(p, head, field)							\
        for (p = GET_CONTAINER((head)->next, typeof(*p), field);		\
             &(p->field) != (head);									\
             p = GET_CONTAINER(p->field.next, typeof(*p), field))

#define LIST_FOREACH_ELEMENT_SAVE(p, n, head, field)					\
		for (p = GET_CONTAINER((head)->next, typeof(*p), field),		\
			 n = GET_CONTAINER((head)->next->next, typeof(*p), field);	\
			 &(p->field) != (head);										\
			 p = n, n = GET_CONTAINER(p->field.next, typeof(*p), field))

/*
 * check the list is empty or not.
 */
#define CHECK_LIST_EMPTY(head)          \
    ((head)->next == (head))

/*
 * check the node is now in list or not in list.
 */
#define CHECK_NODE_INLIST(node)         \
    (((node)->next != NULL) && ((node)->prev != NULL))
#define CHECK_NODE_OUTOFLIST(node)      (!CHECK_NODE_INLIST(node))


static inline void _list_init(struct list_head *head)
{
    head->next = head;
    head->prev = head;
}

static inline void _list_add
(struct list_head *new,
 struct list_head *prev,
 struct list_head *next)
{
    prev->next = new;
    new->prev = prev;
    new->next = next;
    next->prev = new;
}

static inline void _list_remove
(struct list_head *prev,
 struct list_head *next)
{
    prev->next = next;
    next->prev = prev;
}

static inline void list_add_head
(struct list_head *new,
 struct list_head *head)
{
    _list_add(new, head, head->next);
}

static inline void list_add_tail
(struct list_head *new,
 struct list_head *head)
{
	_list_add(new, head->prev, head);
}

typedef int (*list_ele_comp)(struct list_head *a, struct list_head *b);

/* ������� */
static inline void list_add_comp
(struct list_head *new,
 struct list_head *head,
 list_ele_comp comp_func
 )
{
	struct list_head *p;
	LIST_WALK_THROUGH(p, head)
	{
		if (comp_func(p, new) < 0)
			continue;

		break;
	}

	_list_add(new, p->prev, p);
}


static inline void list_remove(struct list_head *rmnode)
{
    ASSERT(CHECK_NODE_INLIST(rmnode));

    _list_remove(rmnode->prev, rmnode->next);

    rmnode->prev = rmnode->next = NULL;
}

static inline struct list_head *list_remove_head(struct list_head *head)
{
    ASSERT(CHECK_LIST_EMPTY(head) == FALSE);
    
    struct list_head *rmnode = head->next;
    _list_remove(head, rmnode->next);

    rmnode->next = rmnode->prev = NULL;
    
    return rmnode;
}

#endif

