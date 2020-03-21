#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#include "typedef.h"
#include "assert.h"


#define barrier() asm volatile("" ::: "memory")

#define P2V(p)		((char *)(p) - (char *)0)
#define V2P(v)		((void *)0 + v)

#define PUBLIC_BITWIDTH_1M         (20)
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

#define INVALID_IDX				(0xFFFFFFFF)

#define MLD_RET_OK						0
#define MLD_RET_TIMEOUT					(-1)

/*
 * do_div() is NOT a C function. It wants to return
 * two values (the quotient and the remainder), but
 * since that doesn't work very well in C, what it
 * does is:
 *
 * - modifies the 64-bit dividend _in_place_
 * - returns the 32-bit remainder
 *
 * This ends up being the most efficient "calling
 * convention" on x86.
 */
#define do_div(n, base)						\
({								\
	unsigned long __upper, __low, __high, __mod, __base;	\
	__base = (base);					\
	asm("" : "=a" (__low), "=d" (__high) : "A" (n));\
	__upper = __high;				\
	if (__high) {					\
		__upper = __high % (__base);		\
		__high = __high / (__base);		\
	}						\
	asm("divl %2" : "=a" (__low), "=d" (__mod)	\
		: "rm" (__base), "0" (__low), "1" (__upper));	\
	asm("" : "=A" (n) : "a" (__low), "d" (__high));	\
	__mod;							\
})

static inline u32 get_upper_range(u32 val, u32 pow2)
{
    return (val + pow2 - 1) & (~(pow2 - 1));
}

static inline u32 get_lower_range(u32 val, u32 pow2)
{
    return (val) & (~(pow2 - 1));
}

/*      x     ---> return
 * 0x00000000 ---> -1
 * 0x00000001 ---> 0
 * 0x00000002 ---> 1
 * 0x00000004 ---> 2
 * 0x00000008 ---> 3
 * 0x00000010 ---> 4
 * 0x00001000 ---> 12
 * 0x80000000 ---> 31
 */
static inline int log2(int x)
{
	int r = 31;

	if (!x)
		return -1;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

#define is_log2(v)		(((v) & (v - 1)) == 0)

/* some x86 special operations. */
typedef union _regbuf {
    struct {
        u32 eax, ebx, ecx, edx;
    } reg;
    u32 buf[8];
} regbuf_u;

void x86_rdmsr(regbuf_u *regbuf);
void x86_wrmsr(regbuf_u *regbuf);
const char *get_date_string(void);
u32 alg_bsch(u32 dst, u32 *array, u32 size);
void sort(void *base, u32 num, u32 size,
	  int (*cmp_func)(const void *, const void *),
	  void (*swap_func)(void *, void *, int size));
void *bsearch(const void *key, const void *base, u32 num, u32 size,
	      int (*cmp)(const void *key, const void *elt));

/*  */
extern struct bootparam *g_bootp;

#endif

