#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#include "typedef.h"
#include "assert.h"


#define barrier() asm volatile("" ::: "memory")

#define PUBLIC_BITWIDTH_1K         (10)
#define PUBLIC_BITWIDTH_4K         (12)

#define PUBLIC_GETMASK(width)       ((0x1 << (width)) - 1)

#define ARRAY_ELEMENT_SIZE(ar)      (sizeof(ar) / sizeof((ar)[0]))

#define SWAP_4B(num)                ((((num) & 0xFF000000) >> 24) |         \
                                     (((num) & 0x00FF0000) >> 8) |          \
                                     (((num) & 0x0000FF00) << 8) |          \
                                     (((num) & 0x000000FF) << 24))

#define SWAP_2B(num)                ((((num) & 0xFF00) >> 8) |             \
                                     (((num) & 0x00FF) << 8))

#define max(x, y) ({				\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })


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

/* some x86 special operations. */
typedef union _regbuf {
    struct {
        u32 eax, ebx, ecx, edx;
    } reg;
    u32 buf[8];
} regbuf_u;

void x86_rdmsr(regbuf_u *regbuf);
void x86_wrmsr(regbuf_u *regbuf);


#endif

