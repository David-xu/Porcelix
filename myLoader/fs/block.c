#include "config.h"
#include "block.h"

#if 0
/* bio_getbuff()
 * this is the low effect implementation.
 *
 * return : the dest buff addr
 */
void* bio_getbuff(bio_t *bio, u32 bkidx)
{
    u32 i;
    bio_cache_t *bio_cache, *free = NULL;

    ASSERT(bio->dev->device_type == DEVTYPE_BLOCK);

    /* search the exist bio_cache */
    LIST_FOREACH_ELEMENT(bio_cache, &(bio->bc_head), list)
    {
        for (i = 0; i < sizeof(bio_cache->logicbk_idx), i++)
        {
            /* if the bkidx has been cached, just return addr */
            if (bio_cache->logicbk_idx[i] == bkidx)
            {
                return (void *)((u32)get_phyaddr(bio_cache->buffpage) + i * LOGIC_BLOCK_SIZE);
            }
            else if ((bio_cache->logicbk_idx[i] == INVALID_LOGICBKIDX) &&
                     (free == NULL))
            {
                free == bio_cache;
            }
        }
    }

    if (free)
    {
        for (i = 0; i < sizeof(bio_cache->logicbk_idx), i++)
    }
}
#endif

