#include "config.h"
#include "typedef.h"
#include "public.h"
#include "module.h"
#include "task.h"
#include "debug.h"

void memcpy(void *dst, void *src, u32 len)
{
    while(len--)
    {
        *((u8 *)dst) = *((u8 *)src);
        dst++;
        src++;
    }
}

void memset(void *dst, u8 value, u32 len)
{
    while(len--)
    {
        *((u8 *)dst) = value;
        dst++;
    }
}

void memset_u16(u16 *dst, u16 value, u16 len)
{
    while(len--)
    {
        *dst++ = value;
    }
}

int strcmp(const char *cs, const char *ct)
{
	unsigned char c1, c2;

	while (1) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
	}
	return 0;
}

int memcmp(void *str1, void *str2, u32 len)
{
    u8 *buff1 = (u8 *)str1;
    u8 *buff2 = (u8 *)str2;

    while (len--)
    {
        if (*buff1 > *buff2)
        {
            return 1;
        }
        else if (*buff1 < *buff2)
        {
            return -1;
        }
        buff1++;
        buff2++;
    }

    return 0;
}

u16 strlen(char *str)
{
    u16 len = 0;
    while (*str++)
        len++;

    return len;
}

u32 str2num(char *str)
{
    u32 ret = 0, base = 10;
    
    if ((str[0] == '0') && ((str[1] == 'x') || (str[1] == 'X')))
    {
        base = 16;
        str += 2;
    }
    while (1)
    {
        char ch = *str++;
        if (ch == 0)
        {
            break;
        }
        ret *= base;
        
        if ((ch >= '0') && (ch <= '9'))
        {
            ret += (ch - '0');
        }
        else if ((ch >= 'a') && (ch <= 'f'))
        {
            ret += (ch - 'a' + 10);
        }
        else if ((ch >= 'A') && (ch <= 'F'))
        {
            ret += (ch - 'A' + 10);
        }
        else
            return 0;
    }

    return ret;
}

int isinteflag(char ch, u8 n_inteflag, char *inteflag)
{
    u32 count;
    for (count = 0; count < n_inteflag; count++)
    {
        if (ch == inteflag[count])
        {
            return 1;
        }
    }
    return 0;
}

int parse_str_by_inteflag(char *str, char *argv[], u8 n_inteflag, char *inteflag)
{
    int argc = 0, flag = 0;
    while (*str)
    {
        if (isinteflag(*str, n_inteflag, inteflag) == 0)
        {
            if (flag == 0)
            {
                argv[argc++] = str;
                flag = 1;
            }
        }
        else
        {
            if (flag == 1)
            {
                *str = 0;
                flag = 0;
            }
        }
        str++;
    }

    return argc;
}

u32 alg_bsch(u32 dst, u32 *array, u32 size)
{
	u32 idx_l = 0, idx_r = size;
	while ((idx_l + 1) != idx_r)
	{
		u32 idx_m = (idx_l + idx_r) / 2;
		if (dst == array[idx_m])
		{
			return idx_m;
		}
		else if (dst < array[idx_m])
		{
			idx_r = idx_m;
		}
		else
		{
			idx_l = idx_m;
		}
	}

	return idx_l;
}


/* some x86 special operation. */
void x86_rdmsr(regbuf_u *regbuf)
{
    asm volatile("rdmsr         \n\t"
                 :"=d"(regbuf->reg.edx), "=a"(regbuf->reg.eax)
                 :"c"(regbuf->reg.ecx));
}

void x86_wrmsr(regbuf_u *regbuf)
{
    asm volatile("wrmsr         \n\t"
                 :
                 :"d"(regbuf->reg.edx), "a"(regbuf->reg.eax), "c"(regbuf->reg.ecx));
}

/* 0xFFFFFFFF
 *		^
 *		.			input param ...
 *		.			return addr
 *		.			old ebp					<___ current ebp
 *		.			local reg protect
 *		.
 * 0x00000000
 */
void die(void)
{
	u32 ebp;
	_cli();
	__asm__ __volatile__ (
		"movl	%%ebp, %%eax		\n\t"
		:"=a"(ebp)
		:
	);

	if (is_taskinit_done())
		dump_stack(ebp);

	printk("Dead...");
	while (1);
}

/*
 * -DMLD_DATE=\"$(shell date +%Y%m%d-%H%M%S)\"
 */
const char *get_date_string(void)
{
	const char *date = MLD_DATE;

	return date;
}



/*
 * this 'sort' function was copied from linux kernel
 */

static void u32_swap(void *a, void *b, int size)
{
	u32 t = *(u32 *)a;
	*(u32 *)a = *(u32 *)b;
	*(u32 *)b = t;
}

static void generic_swap(void *a, void *b, int size)
{
	char t;

	do {
		t = *(char *)a;
		*(char *)a++ = *(char *)b;
		*(char *)b++ = t;
	} while (--size > 0);
}

/**
 * sort - sort an array of elements
 * @base: pointer to data to sort
 * @num: number of elements
 * @size: size of each element
 * @cmp_func: pointer to comparison function
 * @swap_func: pointer to swap function or NULL
 *
 * This function does a heapsort on the given array. You may provide a
 * swap_func function optimized to your element type.
 *
 * Sorting time is O(n log n) both on average and worst-case. While
 * qsort is about 20% faster on average, it suffers from exploitable
 * O(n*n) worst-case behavior and extra memory requirements that make
 * it less suitable for kernel use.
 */

void sort(void *base, u32 num, u32 size,
	  int (*cmp_func)(const void *, const void *),
	  void (*swap_func)(void *, void *, int size))
{
	/* pre-scale counters for performance */
	int i = (num/2 - 1) * size, n = num * size, c, r;

	if (!swap_func)
		swap_func = (size == 4 ? u32_swap : generic_swap);

	/* heapify */
	for ( ; i >= 0; i -= size) {
		for (r = i; r * 2 + size < n; r  = c) {
			c = r * 2 + size;
			if (c < n - size &&
					cmp_func(base + c, base + c + size) < 0)
				c += size;
			if (cmp_func(base + r, base + c) >= 0)
				break;
			swap_func(base + r, base + c, size);
		}
	}

	/* sort */
	for (i = n - size; i > 0; i -= size) {
		swap_func(base, base + i, size);
		for (r = 0; r * 2 + size < i; r = c) {
			c = r * 2 + size;
			if (c < i - size &&
					cmp_func(base + c, base + c + size) < 0)
				c += size;
			if (cmp_func(base + r, base + c) >= 0)
				break;
			swap_func(base + r, base + c, size);
		}
	}
}

/*
 * this 'bsearch' function was copied from linux kernel
 */

/*
 * bsearch - binary search an array of elements
 * @key: pointer to item being searched for
 * @base: pointer to first element to search
 * @num: number of elements
 * @size: size of each element
 * @cmp: pointer to comparison function
 *
 * This function does a binary search on the given array.  The
 * contents of the array should already be in ascending sorted order
 * under the provided comparison function.
 *
 * Note that the key need not have the same type as the elements in
 * the array, e.g. key could be a string and the comparison function
 * could compare the string with the struct's name field.  However, if
 * the key and elements in the array are of the same type, you can use
 * the same comparison function for both sort() and bsearch().
 */
void *bsearch(const void *key, const void *base, u32 num, u32 size,
	      int (*cmp)(const void *key, const void *elt))
{
	u32 start = 0, end = num;
	int result;

	while (start < end) {
		u32 mid = start + (end - start) / 2;

		result = cmp(key, base + mid * size);
		if (result < 0)
			end = mid;
		else if (result > 0)
			start = mid + 1;
		else
			return (void *)base + mid * size;
	}

	return NULL;
}

