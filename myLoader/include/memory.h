#ifndef _MEMORY_H_
#define _MEMORY_H_


#include "public.h"
#include "list.h"
#include "section.h"
#include "interrupt.h"
#include "avl.h"

/* ok, we know that in intel chip, the page size is just 4K */
#define PAGE_SIZE                   (0x1 << PUBLIC_BITWIDTH_4K)
#define PAGESIZE_SHIFT              (PUBLIC_BITWIDTH_4K)
#define EXTENT_RAM_ADDR             (0x100000)

/* system end position */
DEFINE_SYMBOL(symbol_sys_end);
DEFINE_SYMBOL(symbol_inittext_end);


/* we need to call BIOS INT to get some RAM info */
#define RAMINFO_MAXLEN              (0x10)
extern u16 raminfo_buf[RAMINFO_MAXLEN];
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
    BUDDY_RANK_1M,          /* 0x100000 */
    BUDDY_RANK_2M,          /* 0x200000 */
    BUDDY_RANK_4M,          /* 0x400000 */
    BUDDY_RANK_8M,          /* 0x800000 */
    BUDDY_RANK_16M,         /* 0x1000000 */
    BUDDY_RANK_32M,         /* 0x2000000 */
    BUDDY_RANK_64M,         /* 0x4000000 */
    BUDDY_RANKNUM,          /* indicate the biggest continuous block length */
};

#define BUDDY_INVALIDRANK       (0xFFFFFFFF)

struct page{
	struct list_head    freepage;               /*  */
	u32					va;						/* virtual address, when we use the mmu */
	u32                 buddy_rank  : 4;        /*  */
	u32					refcount	: 12;		/* ref count */
	u32                 rsv1        : 16;
};

/* page alloc flag */
#define PAF_AREAMASK	(0x3)					/* 2bit mask */

#define	PAF_ZERO		(0x10)					/* zero value */

/* we start up the free page buddy system. */
void buddy_init(void);

void NMIfault_entry(void);
void DFfault_entry(void);
void TSSfault_entry(void);
void NPfault_entry(void);
void SSfault_entry(void);
void GPfault_entry(void);
void PFfault_entry(void);

void syscall_entry(void);

/*  */

struct _memarea;

void *page_alloc(u32 rank, u32 paf);
void page_free(void *addr);
int sysram_online(u32 begin, u32 size);
u32 get_pagerank(u32 size);
struct _memarea *get_pagearea(struct page *page);
void *page2pa(struct page *page);
struct page *pa2page(void *phy);

/* this is debug function. */
void dump_freepage(void);

/* page directory entry */
typedef struct _pde{
	u32		p			: 1;			/* present flag */
	u32		rw			: 1;			/* r/w flag, 0 indicate read only */
	u32		us			: 1;			/* user/super, 0 indicate super, 1 indicate user */
	u32		pwt			: 1;			/* write-through */
	u32		pcd			: 1;			/* cache disable */
	u32		a			: 1;			/* accessed */
	u32		zero		: 1;
	u32		ps			: 1;			/* page size, 0 indicate 4K */
	u32		g			: 1;			/*  */
	u32		avail		: 3;			/* avaliable for system programmer's use */
	u32		pt_base		: 20;
} pde_t;

/* page table entry */
typedef struct _pte{
	u32		p			: 1;			/* present flag */
	u32		rw			: 1;			/* r/w flag, 0 indicate read only */
	u32		us			: 1;			/* user/super, 0 indicate super, 1 indicate user */
	u32		pwt			: 1;			/* write-through */
	u32		pcd			: 1;			/* cache disable */
	u32		a			: 1;			/* accessed flag */
	u32		d			: 1;			/* dirty flag */
	u32		pat			: 1;			/* page table attribute index */
	u32		g			: 1;			/*  */
	u32		avail		: 3;			/* avaliable for system programmer's use */
	u32		pg_base		: 20;
} pte_t;

/* 每页容纳的页表项数量 */
#define N_PTEENTRY_PERPAGE				(PAGE_SIZE / sizeof(pte_t))
/* normal memory空间对应的页目录表项数量 */
#define NORMALMEM_N_PDEENTRY			(MM_NORMALMEM_RANGE / PAGE_SIZE / N_PTEENTRY_PERPAGE)

/* get CR0 */
static inline u32 getCR0(void)
{
	u32 ret;
	__asm__ __volatile__ (
		"movl	%%cr0, %%eax		\n\t"
		:"=a"(ret)
		:
	);

	return ret;
}
#define	setCR0(cr0)					\
	do {							\
		u32 __cr0 = (cr0);			\
		__asm__ __volatile__ (		\
			"movl	%%eax, %%cr0		\n\t"	\
			:						\
			:"a"(__cr0)				\
		);							\
	} while (0)



/* get CR2 */
static inline u32 getCR2(void)
{
	u32 ret;
	__asm__ __volatile__ (
		"movl	%%cr2, %%eax		\n\t"
		:"=a"(ret)
		:
	);

	return ret;
}


/* get CR3 */
static inline u32 getCR3(void)
{
	u32 ret;
	__asm__ __volatile__ (
		"movl	%%cr3, %%eax			\n\t"
		:"=a"(ret)
		:
	);

	return ret;
}
#define	setCR3(cr3)					\
	do {							\
		u32 __cr3 = cr3;			\
		__asm__ __volatile__ (		\
			"movl	%%eax, %%cr3		\n\t"	\
			:						\
			:"a"(__cr3)				\
		);							\
	} while (0)

#if 0
static inline u32 getGDTR(void)
{
	u32 ret;
	__asm__ __volatile__ (
		"sgdt	%0					\n\t"
		:"=m"(ret)
		:
	);

	return ret;
}

static inline u32 getIDTR(void)
{
	u32 ret;
	__asm__ __volatile__ (
		"sidt	%0					\n\t"
		:"=m"(ret)
		:
	);

	return ret;
}
#endif

void mem_init(void);

int mmap(u32 va, u32 pa, u32 len, u32 usermode);
int mmap1page(u32 va, u32 pa, u32 usermode);

asmlinkage void do_pagefault(struct pt_regs *regs, u32 error_code);
asmlinkage void do_nmifault(struct pt_regs *regs, u32 error_code);
asmlinkage void do_dffault(struct pt_regs *regs, u32 error_code);
asmlinkage void do_tssfault(struct pt_regs *regs, u32 error_code);
asmlinkage void do_npfault(struct pt_regs *regs, u32 error_code);
asmlinkage void do_ssfault(struct pt_regs *regs, u32 error_code);
asmlinkage void do_gpfault(struct pt_regs *regs, u32 error_code);

enum {
	MMAREA_LOW1M = 0,
	MMAREA_NORMAL,
	MMAREA_HIGHMM,
	MMAREA_NUM
};

typedef struct _memarea {
	u8		name[16];
	u32		begin, size;

	/* all free pages in the fpage_list, it's rank has been set */
	struct list_head    fpage_list[BUDDY_RANKNUM];
} memarea_t;

extern memarea_t	g_mmarea[MMAREA_NUM];

#define RANGERESOURCE_TYPE_MASK	0x7
enum {
	RANGERESOURCE_TYPE_IDLE = 0,
	RANGERESOURCE_TYPE_USED,
	RANGERESOURCE_TYPE_RESERVE,
};

typedef struct _range_desc {
	u32		begin, size, flag;
} range_desc_t;

typedef struct _resource {
	u32		n_range;
	range_desc_t	rd[MM_RANGERESOURCE_MAXNUM];
} resource_t;

extern resource_t io_resource;
extern resource_t mem_resource;

int resoure_add_range(resource_t *sr, u32 begin, u32 size, u32 flag);
void dump_resource(resource_t *sr);

/* next free block */
typedef struct _nfblock {
	struct _nfblock		*next;
} nfblock_t;

/* [rg_left, rg_right) */
typedef struct _rg {
	u32	rg_left, rg_right;
} rg_t;

/* slab heap's slab */
typedef struct _slab {
	struct list_head	curslb;					/*  */
	avln_t				spbst;					/* this is the space range BST node */
	rg_t				rg;						/* mem range of current slab */
	nfblock_t			*freepos;				/* next free block possition in this slab */
	u32					n_fblk;					/* number of current free block in this slab */
	/* the block space */
} slab_t;

#define MEMCACHE_NAME_MAXLEN					(32)
typedef struct _memcache {
	char				name[MEMCACHE_NAME_MAXLEN];
	/*
	 * af: all free 
	 * hf: half free
	 * nf: no free
	 */
	struct list_head	afh, hfh, nfh;				/* 3 kinds of slab listhead */
	/*
	 * 'no free' and 'half free' slabs are organized by BST
	 * when free blocks, we find the corresponding slab struct by this BST
	 */
	avln_t				*slbtroot;
	
	u32					r_slb;					/* rank of slab, include the mngt's size */
	u32					s_blk;					/* size of block */
	u32					bps;					/* number of blocks per slab */
	u32					n_blk;					/* number of all blocks in this cache */

	struct list_head	cachelist;				/* 用于串起所有的memcache, 头部是cache_cache中的这个域 */
} memcache_t;

int slab_init(void);
void *memcache_alloc(memcache_t *cache);
void memcache_free(memcache_t *cache, void *blk);
memcache_t *memcache_create(u32 s_blk, u32 r_slb, char *name);
void memcache_destroy(memcache_t *cache);
void dump_memcache(void);

#endif

