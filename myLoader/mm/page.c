#include "typedef.h"
#include "public.h"
#include "config.h"
#include "memory.h"
#include "list.h"
#include "debug.h"
#include "spinlock.h"
#include "command.h"

const char *range_name[BUDDY_RANKNUM] = {"  BUDDY_RANK_4K",
                                         "  BUDDY_RANK_8K",
                                         " BUDDY_RANK_16K",
                                         " BUDDY_RANK_32K",
                                         " BUDDY_RANK_64K",
                                         "BUDDY_RANK_128K",
                                         "BUDDY_RANK_256K",
                                         "BUDDY_RANK_512K",
                                         "  BUDDY_RANK_1M",
                                         "  BUDDY_RANK_2M",
                                         "  BUDDY_RANK_4M",
                                         "  BUDDY_RANK_8M",
                                         " BUDDY_RANK_16M",
                                         " BUDDY_RANK_32M",
                                         " BUDDY_RANK_64M"};

/*  */
static struct page *pagelist = NULL;
u32 n_pages = 0;

#define GET_PAGE_IDX(page)		((u32)((page) - pagelist))
#define GET_BUDDY_PAGE(page)	(pagelist + (GET_PAGE_IDX(page) ^ (0x1 << (page)->buddy_rank)))
/* get the page phy addr by struct page *point */
#define GET_PHYADDR(page)		(GET_PAGE_IDX(page) * PAGE_SIZE)

u32 get_pagerank(u32 size)
{
	int i = log2(size);
	if (i < PAGESIZE_SHIFT)
		return 0;
	if ((0x1 << i) < size)
		return (i - PAGESIZE_SHIFT + 1);
	return (i - PAGESIZE_SHIFT);
}

memarea_t *get_pagearea(struct page *page)
{
	u32 i, pa = GET_PHYADDR(page);
	for (i = 0; i < MMAREA_NUM; i++)
	{
		if (!(g_mmarea[i].size))
			continue;
		if ((pa >= g_mmarea[i].begin) &&
			(pa < (g_mmarea[i].begin + g_mmarea[i].size)))
			return &(g_mmarea[i]);
	}
	ASSERT(0);
	return NULL;		/* can't reach here, make gcc happy */
}

void *page2pa(struct page *page)
{
    return (void *)GET_PHYADDR(page);
}

struct page *pa2page(void *phy)
{
    return (pagelist + (((u32)phy) >> PAGESIZE_SHIFT));
}

/*
static void print_sp(void)
{
    u32 sp = 0;
    __asm__ __volatile__(
    "movl   %%esp, %%eax    \n\t"
    "movl   %%eax, %0       \n\t"
    :"=r"(sp)
    );
    printf("sp:0x%#8x\n", sp);
}
*/

void buddy_init(void)
{   
    /* take the page list after 1M(0x100000) */
    pagelist = (struct page *)EXTENT_RAM_ADDR;
    /* how many RAM we used in page list */
    n_pages = ram_size / PAGE_SIZE;

	/* reserve the pagelist ram range */
	ASSERT(0 == resoure_add_range(&mem_resource, EXTENT_RAM_ADDR,
								   get_upper_range(n_pages * sizeof(struct page), PAGE_SIZE),
								   RANGERESOURCE_TYPE_USED));

    printf("pagelist size:%d(KB).\n", (n_pages * sizeof(struct page)) >> 10);

    /* now init all pages, all of them are NOT in free list, can't be allocated */
	memset(pagelist, 0, n_pages * sizeof(struct page));
}

/* page_alloc()
 * this is the lowest memory management function, just get some continuous
 * pages in the phy ram. We used the buddy alg.
 * rank: 0 --->   1 page  (   4K    0x1000)   7 ---> 128 pages (512KB   0x80000)
 *       1 --->   2 pages (   8K    0x2000)   8 ---> 256 pages (   1M  0x100000)
 *       2 --->   4 pages (  16K    0x4000)   9 ---> 512 pages (   2M  0x200000)
 *       3 --->   8 pages (  32K    0x8000)  10 --->1024 pages (   4M  0x400000)
 *       4 --->  16 pages (  64K   0x10000)  11 --->2048 pages (   8M  0x800000)
 *       5 --->  32 pages (128KB   0x20000)  12 --->4096 pages (  16M 0x1000000)
 *       6 --->  64 pages (256KB   0x40000)  13 --->8192 pages (  32M 0x2000000)
 * paf : 0xXXXXXXX3 mask
 *                0 --> low 1M
 *                1 --> normal
 * return: the phy addr of the free page
 */
void *page_alloc(u32 rank, u32 paf)
{
	u32 flags;
	unsigned i = rank;
	memarea_t *curarea;
	struct page *newpage = NULL;

	ASSERT((paf & PAF_AREAMASK) < MMAREA_NUM);
	curarea = &(g_mmarea[(paf & PAF_AREAMASK)]);

	if (!(curarea->size))
	{
		DEBUG("page_alloc failed, NO space in %s area.\n", curarea->name);
		return NULL;
	}

	DEBUG("page_alloc begin, need rank:%d.\n", rank);
	/* search one free page */
	for (i = rank; i < BUDDY_RANKNUM; i++)
	{
		if (!CHECK_LIST_EMPTY(&(curarea->fpage_list[i])))
			break;
	}

	/**/
	if (i == BUDDY_RANKNUM)
	{
		DEBUG("Current i:0x%#8x.\n", i);
		DEBUG("page_alloc rank %s failed, no enough RAM.\n", range_name[rank]);
		return NULL;
	}

	/* when we found unempty free list, we get one node and remove it
	out of it's freelist */
	DEBUG("Get freepage list, rank:%d.\n", i);
	raw_local_irq_save(flags);
	struct list_head *p = list_remove_head(&(curarea->fpage_list[i]));

	newpage = GET_CONTAINER(p, struct page, freepage);

	/* if the newpage's rank bigger then request, we need to split it
	   and put the second half into free list */
	while (newpage->buddy_rank > rank)
	{
		/* take down the page rank */
		(newpage->buddy_rank)--;

		/* set the rank of the buddy page */
		struct page *buddypage = GET_BUDDY_PAGE(newpage);
		buddypage->buddy_rank = newpage->buddy_rank;

		/* insert the buddy into freelist */
		list_add_head(&(buddypage->freepage),
					  &(curarea->fpage_list[buddypage->buddy_rank]));        
	}

	raw_local_irq_restore(flags);

	/* if alloc highmm, we need to mmap */
	if ((paf & PAF_AREAMASK) == MMAREA_HIGHMM)
	{
		
	}

	/* if with zero flag */
	if (paf & PAF_ZERO)
		memset((void *)GET_PHYADDR(newpage), 0, 0x1 << (rank + PAGESIZE_SHIFT));

	/* finally we get what we want. */
	DEBUG("page_alloc rank %s success.\n", range_name[newpage->buddy_rank]);

	return (void *)GET_PHYADDR(newpage);
}

void page_free(void *addr)
{
    u32 flags;
	struct page *freepage = pa2page(addr);

	/* make sure nobody use this page either */
	ASSERT(freepage->refcount == 0);
	/**/
    ASSERT(freepage->buddy_rank < BUDDY_RANKNUM);
    /* the page should not in the free page list. */
    ASSERT(CHECK_NODE_OUTOFLIST(&(freepage->freepage)));

	/* get the area of this page */
	memarea_t *area = get_pagearea(freepage);

	/* spin lock */
    raw_local_irq_save(flags);

    while (freepage->buddy_rank < (BUDDY_RANKNUM - 1))
    {
        /* if the buddypage is free, and it's rank eq to
           the freepage then combine them. */
        struct page *buddypage = GET_BUDDY_PAGE(freepage);

		if (CHECK_NODE_OUTOFLIST(&(buddypage->freepage)) ||
			(buddypage->buddy_rank != freepage->buddy_rank) ||
			(get_pagearea(buddypage) != area))			/* only combine the save area pages */
		{
			/* we need to insert this page into freelist */
			break;
		}

        /* remove the buddypage out of freelist */
        list_remove(&(buddypage->freepage));
        
        /* upgrade the freepage, increase the rank */
        freepage = (u32)freepage < (u32)buddypage ? freepage : buddypage;
        (freepage->buddy_rank)++;
    }

    /* insert it into freelist */
    list_add_head(&(freepage->freepage), &(area->fpage_list[freepage->buddy_rank]));
    raw_local_irq_restore(flags);
}

int sysram_online(u32 begin, u32 size)
{
	ASSERT((begin & (PAGE_SIZE - 1)) == 0);
	ASSERT((size & (PAGE_SIZE - 1)) == 0);
	struct page *p;
	void *pa = (void *)begin;
	while ((u32)pa < (begin + size))
	{
		p = pa2page(pa);
		p->buddy_rank = 0;
		p->refcount = 0;
		page_free(pa);

		pa += PAGE_SIZE;
	}
	return 0;
}

/* freepage_dump()
 */
void dump_freepage(void)
{
	int i, totalKB, blocknum;
	struct list_head *p;
	memarea_t *area = g_mmarea;
	while (area < &(g_mmarea[MMAREA_NUM]))
	{
		printf("area %s  [0x%#8x--0x%#8x):\n",
			   area->name, area->begin, area->begin + area->size);

		totalKB = 0;
		for (i = 0; i < BUDDY_RANKNUM; i++)
		{
			if (CHECK_LIST_EMPTY(&(area->fpage_list[i])))
			{
				continue;
			}

			blocknum = 0;
			LIST_WALK_THROUGH(p, &(area->fpage_list[i]))
			{
				blocknum++;
			}
			printf("rank %2d(%s): %4d blocks.\n", i, range_name[i], blocknum);
			totalKB += blocknum << (i + 2);
		}
		
		printf("Total memory left: %4dM %4dKB.\n", totalKB >> 10, totalKB & (0x3FF));
		
		area++;
	}
}



