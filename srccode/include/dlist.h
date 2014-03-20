#ifndef _D_LIST_H_
#define _D_LIST_H_

/* double link list node */
struct dlist_node{
    struct dlist_node *next;
    struct dlist_node *priv;
};

#define DLIST_INIT(newnode)     {.next = &newnode, .priv = &newnode}
#define DLIST_NEW(newnode)      struct dlist_node newnode = DLIST_INIT(newnode)

/* get the owner pointer
 * node  :
 * owner : owner->type
 * type  :
 */
#define DLIST_GET_CONTAINER(node, owner, type)                      \
        ((owner *)(((void *)(node)) - (void *)&(((owner *)0)->type)))

/* walk through the dlist
 * walker : owner struct pointer
 * head   : dlist head
 * type   : the type of the dlist_node
 */
#define DLIST_WALKTHROUGH(walker, head, type)                \
        for (walker = DLIST_GET_CONTAINER((head)->next, typeof(*walker), type);         \
             (head) != &(walker->type);                                                 \
             walker = DLIST_GET_CONTAINER(walker->type.next, typeof(*walker), type))

inline void dlist_add(struct dlist_node *newnode, struct dlist_node *head)
{
    newnode->next = head->next;
    newnode->priv = head;
    head->next->priv = newnode;
    head->next = newnode;
}

inline void dlist_del(struct dlist_node *delnode)
{
    delnode->next->priv = delnode->priv;
    delnode->priv->next = delnode->next;
}

#endif

