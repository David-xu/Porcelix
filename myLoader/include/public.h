#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#include "typedef.h"

#define PUBLIC_BITWIDTH_1K         (10)
#define PUBLIC_BITWIDTH_4K         (12)


static inline u32 get_upper_range(u32 val, u32 pow2)
{
    ASSERT((pow2 & (pow2 - 1)) == 0);
    return (val + pow2 - 1) & (~(pow2 - 1));
}

static inline int get_high_bit(u32 v)
{
    int i, flag = 0x80000000;
    for (i = 0; i < (sizeof(u32) * 8); i++)
    {
        if (v & flag)
        {
            return (sizeof(u32) * 8 - 1 - i);
        }
        flag >>= 1;
    }

    return -1;
}

#endif

