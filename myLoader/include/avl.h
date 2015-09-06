#ifndef _AVL_H_
#define _AVL_H_

#include "list.h"

typedef enum _avlcr{
	AVLCMP_1LT2 = 1,	/* v1 < v2 */
	AVLCMP_1EQ2,		/* v1 = v2 */
	AVLCMP_1BT2,		/* v1 > v2 */
	AVLCMP_1IN2,		/* v1 ( v2 */
	AVLCMP_2IN1,		/* v2 ( v1 */
	AVLCMP_OVLP,		/* overlap */
} avlcr_e;

/*
 * some walkthrough type: L(left) M(middle) R(right)
 */
typedef enum _avlwt{
	AVLWT_LMR = 0,		/* first left, then middle, last right */
	AVLWT_LRM,
	AVLWT_MLR,
	AVLWT_MRL,
	AVLWT_RLM,
	AVLWT_RML,
} avlwt_t;

typedef struct _avln {
	struct _avln *f;					/* father node */
	struct _avln *lc, *rc;				/* left and right child node */
	unsigned	subth;					/* subtree high, >= 1 */
} avln_t;

/*
 * Binary Search Tree node compire function
 * cn ----> 1
 * p  ----> 2
 * return: AVLCMP_1LT2
 *         AVLCMP_1EQ2
 *         AVLCMP_1BT2
 *         AVLCMP_1IN2
 *         AVLCMP_2IN1
 *         AVLCMP_OVLP
 */
typedef avlcr_e (*avl_ncmp)(avln_t *cn, void *p);

/*
 * return:  0 ---> walk success
 * return: -1 ---> terminate with some reason
 */
typedef int (*avl_wkop)(avln_t *n1, void *p);

int avl_insert(avln_t **root, avln_t *newnode, avl_ncmp cmpf, void *p);
void avl_remove(avln_t **root, avln_t *rmnode);
avln_t* avl_lkup(avln_t *root, avl_ncmp cmpf, void *p);

/* return: the 'op' function return value */
int avl_walk(avln_t *root, avlwt_t wt, avl_wkop op, void *p);

#endif

