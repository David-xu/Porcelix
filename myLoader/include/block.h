#ifndef _BLOCK_H_
#define _BLOCK_H_

#include "public.h"
#include "device.h"
#include "memory.h"

/* logic block size is 1K */
#define LOGIC_BLOCK_SIZE        (1 << PUBLIC_BITWIDTH_1K)

#define INVALID_LOGICBKIDX      (0xFFFFFFFF)

typedef struct _bio_cache{
    struct page *buffpage;
    u32 logicbk_idx[PAGE_SIZE / LOGIC_BLOCK_SIZE];
    struct list_head list;
}bio_cache_t;

typedef struct _bio{
    device_t *dev;              /* attach to one of the block device */
    struct list_head bc_head;   /* bio_cache_t list head */
}bio_t;

void* bio_getbuff(bio_t *bio, u32 bkidx);

#endif

