#include "public.h"
#include "memory.h"
#include "module.h"

static memcache_t cache_cache;

static int init_onecache(memcache_t *cache, char *name, u32 s_blk, u32 r_slb)
{
	_list_init(&(cache->afh));
	_list_init(&(cache->hfh));
	_list_init(&(cache->nfh));

	memcpy(cache->name, name, strlen(name));

	cache->s_blk 	= (s_blk + sizeof(nfblock_t) - 1) & (~(sizeof(nfblock_t) - 1));
	cache->r_slb	= r_slb;
	cache->bps		= ((0x1 << (r_slb + PAGESIZE_SHIFT)) - sizeof(slab_t)) / cache->s_blk;
	cache->n_blk	= 0;

	cache->slbtroot = NULL;

	_list_init(&(cache->cachelist));

	return 0;
}

/* slab stack new slab */
static void initslab(memcache_t *cache, slab_t *slab)
{
	nfblock_t *fp = (nfblock_t *)(slab + 1);

	slab->rg.rg_left = (u32)fp;
	slab->rg.rg_right = (u32)slab + (0x1 << (cache->r_slb + PAGESIZE_SHIFT));

	/* init number of free blocks */
	slab->n_fblk = cache->bps;

	/* init the free list */
	slab->freepos = NULL;
	/* the first free block's next is NULL */
	while (((u32)fp + cache->s_blk) <= slab->rg.rg_right)
	{
		fp->next = slab->freepos;
		slab->freepos = fp;
		fp = (nfblock_t *)((u32)fp + cache->s_blk);
	}
}

/* slab range compire */
static avlcr_e slabcmp(avln_t *cn, void *p)
{
	slab_t *slab1 = GET_CONTAINER(cn, slab_t, spbst);
	slab_t *slab2 = (slab_t *)p;

	if (slab1->rg.rg_right <= slab2->rg.rg_left)
		return AVLCMP_1LT2;
	else if (slab1->rg.rg_left >= slab2->rg.rg_right)
		return AVLCMP_1BT2;
	else if ((slab1->rg.rg_left == slab2->rg.rg_left) &&
			 (slab1->rg.rg_right == slab2->rg.rg_right))
		return AVLCMP_1EQ2;
	else if ((slab1->rg.rg_left >= slab2->rg.rg_left) &&
			 (slab1->rg.rg_right <= slab2->rg.rg_right))
		return AVLCMP_1IN2;
	else if ((slab1->rg.rg_left <= slab2->rg.rg_left) &&
			 (slab1->rg.rg_right >= slab2->rg.rg_right))
		return AVLCMP_2IN1;

	return AVLCMP_OVLP;
}

/*
 * get one block, filled with zero
 */
void *memcache_alloc(memcache_t *cache)
{
	slab_t *slab;
	nfblock_t *fp = NULL;

	if (!(CHECK_LIST_EMPTY(&(cache->hfh))))		/* likely */
	{
		/* get one slab from half free list */
		slab = GET_CONTAINER(cache->hfh.next, slab_t, curslb);
	}
	else
	{
		/*  */
		if (!(CHECK_LIST_EMPTY(&(cache->afh))))
		{
			/* get one slab from all free list */
			slab = GET_CONTAINER(cache->afh.next, slab_t, curslb);
		}
		else
		{
			slab = (slab_t *)page_alloc(cache->r_slb, MMAREA_NORMAL);
			if (slab == NULL)
			{
				goto cpkl_shalloc_ret;
			}
			initslab(cache, slab);

			/* let's insert this new slab into all free list */
			list_add_head(&(slab->curslb), &(cache->afh));
		}
	}

	/* remove the first block from the freelist */
	fp = slab->freepos;
	slab->freepos = fp->next;

	memset(fp, 0, cache->s_blk);

	/*
	 * if current number of free blocks eq to number of blocks per slab
	 * remove the slab from all free list
	 * and insert it into half free list
	 */
	if (slab->n_fblk == cache->bps)
	{
		list_remove(&(slab->curslb));
		list_add_head(&(slab->curslb), &(cache->hfh));
		/* now the  */
		ASSERT(0 == avl_insert(&(cache->slbtroot), &(slab->spbst), slabcmp, slab));
	}

	(slab->n_fblk)--;

	/*
	 * if current number of free blocks eq to 0
	 * remove the slab from half free list
	 * and insert it into no free list
	 */
	if (slab->n_fblk == 0)
	{
		list_remove(&(slab->curslb));
		list_add_head(&(slab->curslb), &(cache->nfh));
	}

	(cache->n_blk)++;

cpkl_shalloc_ret:

	return (void *)fp;
}
EXPORT_SYMBOL(memcache_alloc);


void memcache_free(memcache_t *cache, void *blk)
{
	/* this is the fake shs, just used to AVL lookup */
	nfblock_t *fp = (nfblock_t *)blk;
	slab_t lkupshs, *dstshs;
	lkupshs.rg.rg_left = (u32)blk;
	lkupshs.rg.rg_right = (u32)blk + cache->s_blk;
	/* find the blk's corrodinate slab by AVL, just save the result in dstshs */
	dstshs = (slab_t *)avl_lkup(cache->slbtroot, slabcmp, &lkupshs);
	/* we can't find blk in AVL, the block is not alloced from this sh */
	if (NULL == dstshs)
	{
		ERROR("memcache_free() error block, addr 0x%#8x in cache %s.\n", (u32)blk, cache->name);
		return;
	}
	dstshs = GET_CONTAINER(dstshs, slab_t, spbst);

	/* insert this block into slab's freelist */
	fp->next = dstshs->freepos;
	dstshs->freepos = fp;

	/*
	 * if current number of free blocks eq to 0
	 * remove the slab from no free list
	 * and insert it into half free list
	 */
	if (dstshs->n_fblk == 0)
	{
		list_remove(&(dstshs->curslb));
		list_add_head(&(dstshs->curslb), &(cache->hfh));
	}
	
	(dstshs->n_fblk)++;

	/*
	 * if current number of free blocks eq to number of blocks per slab
	 * remove the slab from half free list
	 * and insert it into all free list
	 */
	if (dstshs->n_fblk == cache->bps)
	{
		list_remove(&(dstshs->curslb));
		list_add_head(&(dstshs->curslb), &(cache->afh));

		/*
		 * the valid block range CAN'T include the blocks which are in the all free list
		 * so we need to remove the all free slabs from valid range AVL
		 */
		avl_remove(&(cache->slbtroot), &(dstshs->spbst));
	}

	(cache->n_blk)--;
}
EXPORT_SYMBOL(memcache_free);


/*
 * s_blk: size of block
 * r_slb: rand of slab BUDDY_RANK_4K BUDDY_RANK_8K ...
 */
memcache_t *memcache_create(u32 s_blk, u32 r_slb, char *name)
{
	if (strlen(name) >= MEMCACHE_NAME_MAXLEN)
		return NULL;

	memcache_t *ret = (memcache_t *)memcache_alloc(&cache_cache);
	if (ret == NULL)
		return NULL;

	init_onecache(ret, name, s_blk, r_slb);

	/*把这个memcache加入全局memcache链表 注意这里利用了cache_cache中空闲的cachelist作为头 */
	list_add_head(&(ret->cachelist), &(cache_cache.cachelist));

	return ret;
}
EXPORT_SYMBOL(memcache_create);

void memcache_destroy(memcache_t *cache)
{
	if (cache == NULL)
		return;

	slab_t *p, *n;
	/* free all slabs */
	LIST_FOREACH_ELEMENT_SAVE(p, n, &(cache->afh), curslb)		/* all free list */
	{	
		page_free(p);
	}
	LIST_FOREACH_ELEMENT_SAVE(p, n, &(cache->hfh), curslb)		/* half free list */
	{
		page_free(p);
	}
	LIST_FOREACH_ELEMENT_SAVE(p, n, &(cache->nfh), curslb)		/* no free list */
	{
		page_free(p);
	}

	/*
	 * AVL's nodes are all embedded in the slab struct
	 * no need to free
	 */

	/* 把这个memcache从全局链表中摘链 */
	list_remove(&(cache->cachelist));

	memcache_free(&cache_cache, cache);
}
EXPORT_SYMBOL(memcache_destroy);

int slab_init(void)
{
	/* this is the cache's cache, store the memcache_t */
	init_onecache(&cache_cache, "cache_cache", sizeof(memcache_t), BUDDY_RANK_4K);

	return 0;
}

void dump_memcache(void)
{
	memcache_t *p;
	u32 i = 0;
	printf("Totally %d memcache in system:\n"
		   "name:\t\ts_blk\tn_blk\n",
		   cache_cache.n_blk);
	LIST_FOREACH_ELEMENT(p, &(cache_cache.cachelist), cachelist)
	{
		printf("%s\t%d\t%d\n", p->name, p->s_blk, p->n_blk);
		i++;
	}

	ASSERT(i == cache_cache.n_blk);
}

