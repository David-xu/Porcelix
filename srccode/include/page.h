#ifndef _PAGE_H_
#define _PAGE_H_

#include "typedef.h"
#include "dlist.h"

/* x86 page size */
#define PAGESIZE                            (4096)

/* free phy start addr, this var constuct by linker */
extern u32 freephy_start;

/* phy page desc */
struct page{
    /* free list node */
    struct dlist_node   freelist;
};

#endif
