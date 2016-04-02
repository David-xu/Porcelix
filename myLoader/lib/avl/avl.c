#include "avl.h"

#ifdef __MLD__
#include	"assert.h"
#endif

static int avl_inbf(avln_t *node)
{
	ASSERT(node != NULL);

	u32 lch = (node->lc != NULL) ? (node->lc->subth) : 0;
	u32 rch = (node->rc != NULL) ? (node->rc->subth) : 0;
	
	return (lch - rch);
}

/*
 * refrash subtree height
 * return : 1, need to reballance
 *			0, no need
 */
static u32 avl_rfh(avln_t *node)
{
	ASSERT(node != NULL);

	int oldh = node->subth;
	int lch = (node->lc != NULL) ? (node->lc->subth) : 0;
	int rch = (node->rc != NULL) ? (node->rc->subth) : 0;

	node->subth = (lch > rch) ? (lch + 1) : (rch + 1);

	return ((((lch - rch) > 1) || ((lch - rch) < -1)) ||
			(oldh != node->subth));
}

/* avl tree auto balance
 * <-_
 *    \
 *    |    this is ths left rotate
 *    /
 *
 */
static void avl_lr(avln_t **opn)
{
	/* father node field address */
	avln_t *r = *opn;
	avln_t *rc = r->rc;

	rc->f = r->f;

	r->f = rc;
	r->rc = rc->lc;
	if (r->rc)
		r->rc->f = r;
	avl_rfh(r);

	rc->lc = r;
	avl_rfh(rc);

	*opn = rc;
}

/*
 *   _->
 *  /
 *  |      this is the right rotate
 *  \
 */
static void avl_rr(avln_t **opn)
{
	avln_t *r = *opn;
	avln_t *lc = r->lc;

	lc->f = r->f;

	r->f = lc;
	r->lc = lc->rc;
	if (r->lc)
		r->lc->f = r;
	avl_rfh(r);

	lc->rc = r;
	avl_rfh(lc);

	*opn = lc;
}

/*
 * auto ballance opeartion
 * start from the AVL node refer by 'cn'
 */
static void avl_ab(avln_t **root, avln_t *cn)
{
	avln_t **cnp, *wln = cn;

	/* if we insert the root node, no need to change anything either */
	while (wln)
	{
		/* refrash the father node's heigth */
		if (avl_rfh(wln) == 0)
		{
			break;
		}

		/* rotation may be occupyed, we need to store the father node's now */
		cn = wln;
		wln = wln->f;

		if (avl_inbf(cn) < -1)
		{
			if (avl_inbf(cn->rc) > 0)
			{
				avl_rr(&(cn->rc));
			}

			if (cn->f == NULL)
				cnp = root;
			else
				cnp = (cn->f->lc == cn) ? &(cn->f->lc) : &(cn->f->rc);

			avl_lr(cnp);
		}
		else if (avl_inbf(cn) > 1)
		{
			if (avl_inbf(cn->lc) < 0)
			{
				avl_lr(&(cn->lc));
			}

			if (cn->f == NULL)
				cnp = root;
			else
				cnp = (cn->f->lc == cn) ? &(cn->f->lc) : &(cn->f->rc);

			avl_rr(cnp);
		}
	}
}

/*
 * this is the AVL node cut function, without auto ballance
 * treeinfo stored in rmnode should NOT erase
 */
static void avl_ndcut(avln_t **root, avln_t *rmnode)
{
	/* new child after cut */
	avln_t *nc = NULL;

	ASSERT((root != NULL) && (rmnode != NULL));

	if ((rmnode->lc == NULL) && (rmnode->rc == NULL))
	{
		nc = NULL;
	}
	else if (rmnode->lc == NULL)		/* that means rmnode->rc != NULL */
	{
		nc = rmnode->rc;
	}
	else if (rmnode->rc == NULL)		/* that means rmnode->lc != NULL */
	{
		nc = rmnode->lc;
	}
	else
	{
		ASSERT(0);
	}

    if (nc)
	    nc->f = rmnode->f;

	if (rmnode->f != NULL)
	{
		root = (rmnode->f->lc == rmnode) ? &(rmnode->f->lc) : &(rmnode->f->rc);
	}

	*root = nc;
	
}

int avl_insert(avln_t **root, avln_t *newnode, avl_ncmp cmpf, void *p)
{
	ASSERT((root != NULL) && (newnode != NULL));

	avln_t **cnp = root;			/* cur node pointer */
	avln_t *cn = NULL;				/* cur node */

	while (*cnp)
	{
		cn = *cnp;
		switch(cmpf(cn, p))
		{
		case AVLCMP_1LT2:
		{
			cnp = &(cn->rc);
            break;
		}
		case AVLCMP_1BT2:
		{
			cnp = &(cn->lc);
            break;
		}
		default:
			return -1;
		}
	}
	/* we have found the right position, let's save it */
	*cnp = newnode;
	newnode->f = cn;

	/* before auto ballance, the newnode is a leafnode */
	newnode->lc = newnode->rc = NULL;
	newnode->subth = 1;

	avl_ab(root, cn);

	return 0;
}

void avl_remove(avln_t **root, avln_t *rmnode)
{
	avln_t *rbpos;		/* reballance posision */

	if ((rmnode->lc != NULL) && (rmnode->rc != NULL))
	{
		avln_t *rmpos;
	
		/* find the position, substitude the 'rmnode' with the info in rmpos
		 * and cut the rmpos
		 */
		if (rmnode->lc->subth > rmnode->rc->subth)
		{
			/* find the most right child node 
			   in the left subtree */
			rmpos = rmnode->lc;
			while (rmpos->rc != NULL)
				rmpos = rmpos->rc;
		}
		else
		{
			/* find the most left child node 
			   in the right subtree */
			rmpos = rmnode->rc;
			while (rmpos->lc != NULL)
				rmpos = rmpos->lc;
		}

		rbpos = rmpos->f;
        if (rbpos == rmnode)
        {
            rbpos = rmpos;
        }

		/* first we cut the rmpos node */
		avl_ndcut(root, rmpos);

		/* then substitude the rmnode with rmpos
		 * it means rmpos sit on the rmnode's position
		 */
		rmpos->f = rmnode->f;
		rmpos->lc = rmnode->lc;
		rmpos->rc = rmnode->rc;
		rmpos->subth = rmnode->subth;
		if (rmpos->lc)
			rmpos->lc->f = rmpos;
		if (rmpos->rc)
			rmpos->rc->f = rmpos;

		if (rmnode->f != NULL)
		{
			if (rmnode->f->lc == rmnode)
				rmnode->f->lc = rmpos;
			else
				rmnode->f->rc = rmpos;
		}
		else
			*root = rmpos;
	}
	else
	{
		rbpos = rmnode->f;
		
		/* now we just cut the rmnode directly */
		avl_ndcut(root, rmnode);
	}

	/* auto ballance */
	avl_ab(root, rbpos);
}

avln_t* avl_lkup(avln_t *root, avl_ncmp cmpf, void *p)
{
	int cmpret = 0;
	/* use the 'root'var as the walker pointer directlly */
	while (root)
	{
		cmpret = cmpf(root, p);
		switch (cmpret)
		{
		case AVLCMP_1LT2:
			root = root->rc;
			break;
		case AVLCMP_1BT2:
			root = root->lc;
            break;
		case AVLCMP_2IN1:
		case AVLCMP_1EQ2:
			return root;
        default:
			return NULL;
		}
	}

	return NULL;
}

int avl_walk(avln_t *root, avlwt_t wt, avl_wkop op, void *p)
{
	if (root == NULL)
		return 0;

	if (root->subth == 1)
	{
		return op(root, p);
	}

	switch (wt)
	{
	case AVLWT_LMR:
	case AVLWT_LRM:
	{
		if (avl_walk(root->lc, wt, op, p) == -1)
			return -1;
		if (wt == AVLWT_LMR)
		{
			if (op(root, p) == -1)
				return -1;
			if (avl_walk(root->rc, wt, op, p) == -1)
				return -1;
		}
		else
		{
			if (avl_walk(root->rc, wt, op, p) == -1)
				return -1;
			if (op(root, p) == -1)
				return -1;
		}

		break;
	}
	case AVLWT_MLR:
	case AVLWT_MRL:
	{
		if (op(root, p) == -1)
			return -1;

		if (wt == AVLWT_MLR)
		{
			if (avl_walk(root->lc, wt, op, p) == -1)
				return -1;
			if (avl_walk(root->rc, wt, op, p) == -1)
				return -1;
		}
		else
		{
			if (avl_walk(root->rc, wt, op, p) == -1)
				return -1;
			if (avl_walk(root->lc, wt, op, p) == -1)
				return -1;
		}

		break;
	}
	case AVLWT_RLM:
	case AVLWT_RML:
	{
		if (avl_walk(root->rc, wt, op, p) == -1)
			return -1;

		if (wt == AVLWT_RLM)
		{
			if (avl_walk(root->lc, wt, op, p) == -1)
				return -1;
			if (op(root, p) == -1)
				return -1;
		}
		else
		{
			if (op(root, p) == -1)
				return -1;
			if (avl_walk(root->lc, wt, op, p) == -1)
				return -1;
		}

		break;
	}
	default:
		return -1;
	}

    return 0;
}

