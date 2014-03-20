#include "typedef.h"
#include "public.h"
#include "config.h"
#include "memory.h"
#include "list.h"
#include "debug.h"

const u8 *range_name[BUDDY_RANKNUM] = {"BUDDY_RANK_4K",
                                       "BUDDY_RANK_8K",
                                       "BUDDY_RANK_16K",
                                       "BUDDY_RANK_32K",
                                       "BUDDY_RANK_64K",
                                       "BUDDY_RANK_128K",
                                       "BUDDY_RANK_256K",
                                       "BUDDY_RANK_512K",
                                       "BUDDY_RANK_1024K"};

/*  */
struct page *pagelist = NULL;
u32 n_pages = 0;
/* all pages in the fpage_list, it's rank has been set */
struct list_head    fpage_list[BUDDY_RANKNUM];

#define GET_FREEPAGE_IDX(page)  ((page) - pagelist)
#define GET_BUDDY_PAGE(page)    (pagelist + (GET_FREEPAGE_IDX(page) ^ (0x1 << (page)->buddy_rank)))
/* get the page phy addr by struct page *point */
#define GET_PHYADDR(page)       (GET_FREEPAGE_IDX(page) * PAGE_SIZE)

void *get_phyaddr(struct page *page)
{
    return (void *)GET_PHYADDR(page);
}


void buddy_init(void)
{
    u32 freepage_start, i;

    /* init the fpage_list */
    for (i = 0; i < BUDDY_RANKNUM; i++)
        _list_init(&(fpage_list[i]));
    
    /* take the page list after the whole system */
    //pagelist = (struct page *)((GET_SYMBOLVALUE(symbol_sys_end) + MM_HOLEAFTEL_SYSRAM + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1)));
    /* take the page list after 1M(0x100000) */
    pagelist = (struct page *)EXTENT_RAM_ADDR;
    /* how many RAM we used in page list */
    n_pages = ram_size / PAGE_SIZE;

    /* PAGE allign, the free pages we can alloced start from this value */
    freepage_start = ((u32)pagelist + n_pages * sizeof(struct page) + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1));

    printf("pagelist size:%d(KB).\nfreepage_start:0x%#8x\n", n_pages * sizeof(struct page) >> 10, freepage_start);

    /* now init all page block */
    for (i = 0; i < n_pages; i += (0x01 << (BUDDY_RANKNUM - 1)))
    {
        (pagelist + i)->buddy_rank = BUDDY_RANKNUM - 1;
        list_add_tail(&((pagelist + i)->freepage), &(fpage_list[BUDDY_RANKNUM - 1]));
    }
    /* we locate the RAM space */
    /* 0. system space, all pages we alloc will not used this space */
    page_alloc(BUDDY_RANK_512K);
    page_alloc(BUDDY_RANK_128K);
    /* 1. 0xA0000~0x100000, some bios space we should NOT used */
    page_alloc(BUDDY_RANK_128K);
    page_alloc(BUDDY_RANK_256K);
    /* 2. sys page list space */
    for (i = EXTENT_RAM_ADDR; i < freepage_start;)
    {
        
        struct page *tmp;
        if ((freepage_start - i) > 1024 * 1024)
        {
            tmp = page_alloc(BUDDY_RANK_1024K);
            i += 1024 * 1024;
        }
        else
        {
            int mosthigh = get_high_bit(freepage_start - i);
            tmp = page_alloc(mosthigh - PAGESIZE_SHIFT);
            i += 0x1 << mosthigh;
        }
    }
}

/* page_alloc()
 * this is the lowest memory management function, just get some continuous
 * pages in the phy ram. We used the buddy alg.
 * rank: 0 --->  1 page  (4K  0x1000)   5 --->  32 pages (128KB 0x20000)
 *       1 --->  2 pages (8K  0x2000)   6 --->  64 pages (256KB 0x40000)
 *       2 --->  4 pages (16K 0x4000)   7 ---> 128 pages (512KB 0x80000)
 *       3 --->  8 pages (32K 0x8000)   8 ---> 256 pages (1M    0x100000)
 *       4 ---> 16 pages (64K 0x10000)
 * return: the phy addr of the free page
 */
struct page *page_alloc(u32 rank)
{
    unsigned i = rank, pageidx;
    struct page *newpage = NULL;

    /* search one free page */
    for (i = rank; i < BUDDY_RANKNUM; i++)
    {
        if (!CHECK_LIST_EMPTY(&(fpage_list[i])))
            break;
    }
    /**/
    if (i == BUDDY_RANKNUM)
    {
        DEBUG("page_alloc rank %s failed, no enough RAM.\n", range_name[rank]);
        return NULL;
    }
        
    /* when we found unempty free list, we get one node and remove it
       out of it's freelist */
    struct list_head *p = list_remove_head(&(fpage_list[i]));
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
        list_add_head(&(buddypage->freepage), &(fpage_list[buddypage->buddy_rank]));
    }

    /* finally we get what we want. */
    DEBUG("page_alloc rank %s success.\n", range_name[newpage->buddy_rank]);
    return newpage;
}

void page_free(struct page *freepage)
{
    ASSERT(freepage->buddy_rank < BUDDY_RANKNUM);
    /* the page should not in the free page list. */
    ASSERT(CHECK_NODE_OUTOFLIST(&(freepage->freepage)));

    while (freepage->buddy_rank < (BUDDY_RANKNUM - 1))
    {
        /* if the buddypage is free, and it's rank eq to
           the freepage then combine them. */
        struct page *buddypage = GET_BUDDY_PAGE(freepage);
        if (CHECK_NODE_INLIST(&(buddypage->freepage)) &&
            (buddypage->buddy_rank == freepage->buddy_rank))
        {
            /* remove the buddypage out of freelist */
            list_remove(&(buddypage->freepage));
            
            /* upgrade the freepage, increase the rank */
            freepage = (u32)freepage < (u32)buddypage ? freepage : buddypage;
            (freepage->buddy_rank)++;
        }
        else    /* if not, this is the page we need to insert */
        {
            break;
        }
    }

    /* insert it into freelist */
    list_add_head(&(freepage->freepage), &(fpage_list[freepage->buddy_rank]));
}

/* freepage_dump()
 * detail: display information
 * return: total RAM left size(KB)
 */
u32 freepage_dump(int detail)
{
    int i, totalKB = 0;
    for (i = 0; i < BUDDY_RANKNUM; i++)
    {
        if (CHECK_LIST_EMPTY(&(fpage_list[i])))
        {
            continue;
        }
        struct list_head *p;
        int blocknum = 0;
        LIST_WALK_THROUTH(p, &(fpage_list[i]))
        {
            blocknum++;
        }
        if (detail)
            printf("rank %d(0x%#8x): %4d blocks.\n", i, PAGE_SIZE << i, blocknum);
        totalKB += blocknum << (i + 2);
    }
    if (detail)
        printf("Total memory left: %4dM %4dKB.\n", totalKB >> 10, totalKB & (0x3FF));
    return totalKB;
}

