#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "public.h"
#include "list.h"
#include "section.h"

/* ok, we know that in intel chip, the page size is just 4K */
#define PAGE_SIZE                   (0x1 << PUBLIC_BITWIDTH_4K)
#define PAGESIZE_SHIFT              (PUBLIC_BITWIDTH_4K)
#define EXTENT_RAM_ADDR             (0x100000)

/* system end position */
DEFINE_SYMBOL(symbol_sys_end);
DEFINE_SYMBOL(symbol_inittext_end);


/* we need to call BIOS INT to get some RAM info */
#define RAMINFO_MAXLEN              (0x10)
extern u8 raminfo_buf[RAMINFO_MAXLEN];
extern u32 ram_size;    /* this is the whole RAM size, we get it by BIOS INT */

enum BUDDY_RANK{
    BUDDY_RANK_4K = 0,
    BUDDY_RANK_8K,          /* 0x2000   */
    BUDDY_RANK_16K,         /* 0x4000   */
    BUDDY_RANK_32K,         /* 0x8000   */
    BUDDY_RANK_64K,         /* 0x10000  */
    BUDDY_RANK_128K,        /* 0x20000  */
    BUDDY_RANK_256K,        /* 0x40000  */
    BUDDY_RANK_512K,        /* 0x80000  */
    BUDDY_RANK_1024K,       /* 0x100000 */
    BUDDY_RANKNUM,          /* indicate the biggest continuous block length */
};

#define BUDDY_INVALIDRANK       (0xFFFFFFFF)

struct page{
    struct list_head    freepage;               /*  */
    u32                 buddy_rank  : 8;        /*  */
    u32                 rsv1        : 24;
};

/* we start up the free page buddy system. */
void mem_init(void) _SECTION_(.init.text);
void buddy_init(void) _SECTION_(.init.text);

/*  */
struct page *page_alloc(u32 rank);
void page_free(struct page *freepage);
void *get_phyaddr(struct page *page);

/* this is debug function. */
u32 freepage_dump(int detail);

#endif

