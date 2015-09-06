#include "typedef.h"
#include "memory.h"
#include "debug.h"
#include "assert.h"
#include "command.h"
#include "debug.h"
#include "ml_string.h"
#include "module.h"

static void cmd_ramrw_opfunc(char *argv[], int argc, void *param)
{
    u32 addr, len_val;
    if ((argc < 3) || (argc > 4))
    {
        goto invalidparam;
    }
    else if (argc == 3)
    {
    	addr = str2num(argv[2]);
        if (memcmp("-rb", argv[1], sizeof("-rb")) == 0)
        {
            len_val = *((u8 *)addr);
            printf("Read byte from 0x%#8x:0x%#2x.\n", addr, (u8)len_val);
        }
        else if (memcmp("-rw", argv[1], sizeof("-rw")) == 0)
        {
            len_val = *((u16 *)addr);
            printf("Read word from 0x%#8x:0x%#4x.\n", addr, (u16)len_val);
        }
        else if (memcmp("-rl", argv[1], sizeof("-rl")) == 0)
        {
            len_val = *((u32 *)addr);
            printf("Read long from 0x%#8x:0x%#8x.\n", addr, len_val);
        }
        else
        {
            goto invalidparam;
        }
    }
    else if (argc == 4)
    {
    	addr = str2num(argv[2]);
    	len_val = str2num(argv[3]);
        if (memcmp("-r", argv[1], sizeof("-r")) == 0)
        {
        	dump_ram((void *)addr, len_val);
        }
        else if (memcmp("-wb", argv[1], sizeof("-wb")) == 0)
        {
			*((u8 *)addr) = (u8)len_val;
        }
        else if (memcmp("-ww", argv[1], sizeof("-ww")) == 0)
        {
			*((u16 *)addr) = (u16)len_val;
        }
        else if (memcmp("-wl", argv[1], sizeof("-wl")) == 0)
        {
			*((u32 *)addr) = (u32)len_val;
        }
        else
        {
            goto invalidparam;
        }
    }
    else
    {
        goto invalidparam;
    }
    return;

invalidparam:
    printf("-r[b][w][l] [addr]      : Read [BYTE WORD LONG] value from addr.\n"
		   "-r          [addr] [len]: Dump RAM value from addr.\n"
           "-w[b][w][l] [addr] [val]: Write RAM value to addr.\n");
}

struct command cmd_ramrw _SECTION_(.array.cmd) =
{
    .cmd_name   = "ramrw",
    .info       = "RAM read write. -r [addr] [len], -w [addr] [val]",
    .op_func    = cmd_ramrw_opfunc,
};

static void cmd_iorw_opfunc(char *argv[], int argc, void *param)
{
    u32 addr, val;
    if ((argc < 3) || (argc > 4))
    {
        goto invalidparam;
    }
    else if (argc == 3)
    {
        if (memcmp("-rb", argv[1], sizeof("-rb")) == 0)
        {
            addr = str2num(argv[2]);
            val = inb((u16)addr);
            printf("Read byte from I/O 0x%#4x:0x%#2x.\n", (u16)addr, (u8)val);
        }
        else if (memcmp("-rw", argv[1], sizeof("-rw")) == 0)
        {
            addr = str2num(argv[2]);
            val = inw((u16)addr);
            printf("Read word from I/O 0x%#4x:0x%#4x.\n", (u16)addr, (u16)val);
        }
        else if (memcmp("-rl", argv[1], sizeof("-rl")) == 0)
        {
            addr = str2num(argv[2]);
            val = inl((u16)addr);
            printf("Read long from I/O 0x%#4x:0x%#8x.\n", (u16)addr, val);
        }
        else
        {
            goto invalidparam;
        }
    }
    else if (argc == 4)
    {
        if (memcmp("-wb", argv[1], sizeof("-wb")) == 0)
        {
            addr = str2num(argv[2]);
            val = str2num(argv[3]);

            outb((u8)val, (u16)addr);
        }
        else if (memcmp("-ww", argv[1], sizeof("-ww")) == 0)
        {
            addr = str2num(argv[2]);
            val = str2num(argv[3]);

            outw((u16)val, (u16)addr);
        }
        else if (memcmp("-wl", argv[1], sizeof("-wl")) == 0)
        {
            addr = str2num(argv[2]);
            val = str2num(argv[3]);

            outl(val, (u16)addr);
        }
        else
        {
            goto invalidparam;
        }
    }
    else
    {
        goto invalidparam;
    }
    return;

invalidparam:
    printf("-r[b][w][l] [addr]      : Read I/O value from addr.\n"
           "-w[b][w][l] [addr] [val]: write I/O value to addr.\n");
}

struct command cmd_iorw _SECTION_(.array.cmd) =
{
    .cmd_name   = "iorw",
    .info       = "I/O Read Write.-r [addr], -w [addr] [val]",
    .op_func    = cmd_iorw_opfunc,
};


/*
 we call the int 0x15 (ax = 0xE801), get some ram info
 we save the ax, bx, cx, dx.
--------b-15E801-----------------------------
INT 15 - Phoenix BIOS v4.0 - GET MEMORY SIZE FOR >64M CONFIGURATIONS
	AX = E801h
Return: CF clear if successful
	    AX = extended memory between 1M and 16M, in K (max 3C00h = 15MB)
	    BX = extended memory above 16M, in 64K blocks
	    CX = configured memory 1M to 16M, in K
	    DX = configured memory above 16M, in 64K blocks
	CF set on error
Notes:	supported by the A03 level (6/14/94) and later XPS P90 BIOSes, as well
	  as the Compaq Contura, 3/8/93 DESKPRO/i, and 7/26/93 LTE Lite 386 ROM
	  BIOS
	supported by AMI BIOSes dated 8/23/94 or later
	on some systems, the BIOS returns AX=BX=0000h; in this case, use CX
	  and DX instead of AX and BX
	this interface is used by Windows NT 3.1, OS/2 v2.11/2.20, and is
	  used as a fall-back by newer versions if AX=E820h is not supported
	this function is not used by MS-DOS 6.0 HIMEM.SYS when an EISA machine
	  (for example with parameter /EISA) (see also MEM F000h:FFD9h), or no
	  Compaq machine was detected, or parameter /NOABOVE16 was given.
 */
struct raminfo{
    u16     ax, bx, cx, dx;
} __attribute__((packed));
u16 raminfo_buf[RAMINFO_MAXLEN] _SECTION_(.coreentry.param);

u32 ram_size;           /* we call the bios to get this param. */

static void a20enable_test(void) _SECTION_(.init.text);
static void a20enable_test(void)
{
    u32 *p1 = (u32 *)0x100000, *p2 = (u32 *)0x0, v1, v2;
    v1 = *p1;
    v2 = *p2;
    DEBUG("addr 0x%#8x ---> 0x%#8x, addr 0x%#8x ---> 0x%#8x.\n",
           (u32)p1, v1,
           (u32)p2, v2);
    *p1 = 0x5500ffaa;

    v1 = *p1;
    v2 = *p2;
    DEBUG("addr 0x%#8x ---> 0x%#8x, addr 0x%#8x ---> 0x%#8x.\n",
           (u32)p1, v1,
           (u32)p2, v2);
    if (v1 == v2)
    {
        printf("a20 test faild.\n");
        die();
    }
    printf("a20 test success.\n");
}

asmlinkage void do_pagefault(struct pt_regs *regs, u32 error_code)
{
	u32 cr2 = getCR2();			/* 出错地址 */
#if 1
	dump_ptregs(regs);
	printf("PF fault. error_code:0x%#8x, error addr:0x%#8x\n", error_code, cr2);

#if 0
	static int i = 0;
	if (i = 0)
	{
		i = 1;
	}
	else
		while (1);
#endif

	die();
#else
	cr2 &= ~0xFFF;
	mmap(cr2, cr2, 0x1000, 0);
#endif
}

asmlinkage void do_nmifault(struct pt_regs *regs, u32 error_code)
{
	dump_ptregs(regs);
	printf("NMI fault. error_code:0x%#8x\n", error_code);
	die();
}

asmlinkage void do_dffault(struct pt_regs *regs, u32 error_code)
{
	dump_ptregs(regs);
	printf("DF fault. error_code:0x%#8x\n", error_code);
	die();
}

asmlinkage void do_tssfault(struct pt_regs *regs, u32 error_code)
{
	dump_ptregs(regs);
	printf("TSS fault. error_code:0x%#8x\n", error_code);
	die();
}

asmlinkage void do_npfault(struct pt_regs *regs, u32 error_code)
{
	dump_ptregs(regs);
	printf("NP fault. error_code:0x%#8x\n", error_code);
	die();
}

asmlinkage void do_ssfault(struct pt_regs *regs, u32 error_code)
{
	dump_ptregs(regs);
	printf("SS fault. error_code:0x%#8x\n", error_code);
	die();
}

asmlinkage void do_gpfault(struct pt_regs *regs, u32 error_code)
{
	dump_ptregs(regs);
	printf("GP fault. error_code:0x%#8x\n", error_code);
	die();
}

static void __init pgmode_enable(void)
{
	/* let's get one page for the pgd */

	/* make sure the page table all in normal area */
	
	u32 pa = 0, i, cr3 = (u32)page_alloc(BUDDY_RANK_4K, MMAREA_NORMAL | PAF_ZERO);
	u32 ptebuf_rank;
	pde_t *pde = (pde_t *)cr3;
	pte_t *pte;


	/* do the low MM_NORMALMEM_RANGE va mmap to pa */
	ptebuf_rank = log2(MM_NORMALMEM_RANGE >> PUBLIC_BITWIDTH_1M);
	pte = (pte_t *)page_alloc(ptebuf_rank, MMAREA_NORMAL | PAF_ZERO);

	while (pa < MM_NORMALMEM_RANGE)
	{
		if ((((u32)pte) & (PAGE_SIZE - 1)) == 0)
		{
			pde->pt_base = ((u32)pte) >> PAGESIZE_SHIFT;
			pde->us = 0;
			pde->rw = 1;
			pde->p = 1;

			pde++;
		}

		pte->pg_base = pa >> PAGESIZE_SHIFT;
		pte->us = 0;
		pte->rw = 1;
		pte->p = 1;

		pte++;

		pa += PAGE_SIZE;
	}

	setCR3(cr3);

	/* enable pg */
	i = getCR0();
	setCR0(i | 0x80000000);
}

/*
 * free area management
 */
memarea_t	g_mmarea[MMAREA_NUM] = {
	[MMAREA_LOW1M] = {"LOW 1M", 0, 0x1 << PUBLIC_BITWIDTH_1M},
	[MMAREA_NORMAL] = {"NORMAL"},
	[MMAREA_HIGHMM] = {"HIGHMM"},
};

static void mmarea_init(void)
{
	u32 i, j;

	ASSERT(ram_size > g_mmarea[MMAREA_LOW1M].size);
	
	g_mmarea[MMAREA_NORMAL].begin = g_mmarea[MMAREA_LOW1M].size;
	g_mmarea[MMAREA_NORMAL].size = ((MM_NORMALMEM_RANGE < ram_size) ? MM_NORMALMEM_RANGE : ram_size)
									- g_mmarea[MMAREA_LOW1M].size;

	/* with highmm */
	if (ram_size > MM_NORMALMEM_RANGE)
	{
		g_mmarea[MMAREA_HIGHMM].begin = MM_NORMALMEM_RANGE;
		g_mmarea[MMAREA_HIGHMM].size = ram_size - MM_NORMALMEM_RANGE;
	}
	/* else, without highmm */
	
    /* init the fpage_list */
	for (i = 0; i < MMAREA_NUM; i++)		
	    for (j = 0; j < BUDDY_RANKNUM; j++)
	        _list_init(&(g_mmarea[i].fpage_list[j]));
}

/* global iomem resource */
resource_t io_resource = {0};
EXPORT_SYMBOL(io_resource);

resource_t mem_resource = {0};
EXPORT_SYMBOL(mem_resource);

static int rr_cmp(const void *a, const void *b)
{
	range_desc_t *rd1 = (range_desc_t *)a;
	range_desc_t *rd2 = (range_desc_t *)b;

	if (rd1->begin < rd2->begin)
		return -1;
	else if (rd1->begin > rd2->begin)
		return 1;

	return 0;
}

int resoure_add_range(resource_t *sr, u32 begin, u32 size, u32 flag)
{
	u32 i, cur_size = 0, cur_flag = 0;
	u64 cur_begin = 0;

	if ((sr->n_range + 2) > MM_RANGERESOURCE_MAXNUM)
		return -2;
	
	/* find place where to insert this new range */
	for (i = 0; i < sr->n_range; i++)
	{
		if ((sr->rd[i].flag & RANGERESOURCE_TYPE_MASK) != RANGERESOURCE_TYPE_IDLE)
			continue;
		if ((sr->rd[i].begin <= begin) &&
			(sr->rd[i].begin + sr->rd[i].size >= (begin + size)))
		{
			cur_begin = sr->rd[i].begin;
			cur_size = sr->rd[i].size;
			cur_flag = sr->rd[i].flag;
			break;
		}
	}
	/* no such resource range can be used */
	if (i == sr->n_range)
		return -1;

	sr->rd[i].begin = begin;
	sr->rd[i].size = size;
	sr->rd[i].flag = flag;

	/* there is some hole beyond current range, add idle */
	if (begin > cur_begin)
	{
		sr->rd[sr->n_range].begin = cur_begin;
		sr->rd[sr->n_range].size = begin - cur_begin;
		sr->rd[sr->n_range].flag = RANGERESOURCE_TYPE_IDLE;
		(sr->n_range)++;
	}
		
	/* there is some hole behind current range, add idle */
	if ((begin + size) < (cur_begin + cur_size))
	{
		sr->rd[sr->n_range].begin = begin + size;
		sr->rd[sr->n_range].size = cur_begin + cur_size - begin - size;
		sr->rd[sr->n_range].flag = RANGERESOURCE_TYPE_IDLE;
		(sr->n_range)++;
	}

	/* at last, we need to sort all range */
	sort(sr->rd, sr->n_range, sizeof(range_desc_t), rr_cmp, NULL);

	return 0;
}

void dump_resource(resource_t *sr)
{
	u32 i;
	for (i = 0; i < sr->n_range; i++)
		printf("mem [0x%#8x : 0x%#8x)  0x%#8x.\n",
			   sr->rd[i].begin, sr->rd[i].begin + sr->rd[i].size,
			   sr->rd[i].flag);
}

static void cmd_memstat_opfunc(char *argv[], int argc, void *param)
{
	if (argc == 1)
	{
		goto print_usage;
	}
	else if (argc == 2)
	{
		/* page usage state */
		if (memcmp(argv[1], "-p", strlen("-p")) == 0)
		{
			dump_freepage();
		}
		/* mem_resource state */
		else if (memcmp(argv[1], "-mr", strlen("-mr")) == 0)
		{
			dump_resource(&mem_resource);
		}
		else if (memcmp(argv[1], "-c", strlen("-c")) == 0)
		{
			dump_memcache();
		}
		else
			goto print_usage;
	}
	else
		goto print_usage;

	return;

print_usage:
	printf("memstat -p,  stat page\n"
		   "        -mr, stat mem resource\n"
		   "        -c,  stat mem cache\n");
}

struct command cmd_pagestat _SECTION_(.array.cmd) =
{
    .cmd_name   = "memstat",
    .info       = "Memory Statistic.",
    .op_func    = cmd_memstat_opfunc,
};

/******************************************************************************
 *               (1)                (2)
 *   (.init.text) |     system       |   hole   |   page_list   |
 *  (1). value of symbol_inittext_end
 *  (2). value of symbol_sys_end
 *  
 ******************************************************************************/
void mem_init(void)
{
    /* we need to turn on the A20 in some hardware. */
    u8 outport_cmd = kbdctrl_outport_r();
    if ((outport_cmd & 0x02) == 0)
    {
        kbdctrl_outport_w(outport_cmd | 0x02);
        printf("Just turn on the A20.\n");
    }

    a20enable_test();

    /* system ram space + extend ram space */
	struct raminfo *raminfo = (struct raminfo *)(void *)raminfo_buf;
    ram_size = 1024 * 1024 + 
               raminfo->ax * 1024 + 
               raminfo->bx * 1024 * 64;

    printf("Detected ram size is : %dM %dK, ", ram_size >> 20, (ram_size >> 10) & 0x3FF);
	ram_size = get_lower_range(ram_size, PAGE_SIZE);
    printf("we used %dM %dK\n", ram_size >> 20, (ram_size >> 10) & 0x3FF);

	/* init the whole mem resource */
	mem_resource.rd[mem_resource.n_range].begin = 0;
	mem_resource.rd[mem_resource.n_range].size = ram_size;
	mem_resource.rd[mem_resource.n_range].flag = RANGERESOURCE_TYPE_IDLE;
	(mem_resource.n_range)++;

    printf("Total RAM space: %dM.\n", ram_size >> 20);
    printf("The section '.init.text' end position: %#8x.\n", GET_SYMBOLVALUE(symbol_inittext_end));
    printf("System end position: %#8x, Total size: %dK\n", GET_SYMBOLVALUE(symbol_sys_end), GET_SYMBOLVALUE(symbol_sys_end) / 1024);
	/* init the system ram range, we add some hole space after the system */
	ASSERT(0 == resoure_add_range(&mem_resource, 0,
								   get_upper_range(GET_SYMBOLVALUE(symbol_sys_end) + MM_HOLEAFTER_SYSRAM, PAGE_SIZE),
								   RANGERESOURCE_TYPE_USED));

	/* reserve the BIOS ram range */
	ASSERT(0 == resoure_add_range(&mem_resource, 0xA0000, 0x60000, RANGERESOURCE_TYPE_RESERVE));
	/* area init */
	mmarea_init();

    /* build the free page buddy system. */
    buddy_init();

	/*************************************************************************
	 *  now we insert some SYS RAM we can alloc into buddy system.           *
	 *************************************************************************/
	u32 i;
	for (i = 0; i < mem_resource.n_range; i++)
	{
		if ((mem_resource.rd[i].flag & RANGERESOURCE_TYPE_MASK) == RANGERESOURCE_TYPE_IDLE)
			sysram_online(mem_resource.rd[i].begin, mem_resource.rd[i].size);
	}

	slab_init();

	/* ok, let's enable the pg mode */
	pgmode_enable();
}


/*
 * va: it should larger than MM_NORMALMEM_RANGE, because we have mmaped the low MM_NORMALMEM_RANGE va to pa
 */
int mmap(u32 va, u32 pa, u32 len, u32 usermode)
{
	ASSERT(((va & (PAGE_SIZE - 1)) == 0) && ((pa & (PAGE_SIZE - 1)) == 0));
	ASSERT(va < MM_NORMALMEM_RANGE);

	len = (len + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1));

	while (len)
	{
		mmap1page(va, pa, usermode);

		va += PAGE_SIZE;
		pa += PAGE_SIZE;
		len -= PAGE_SIZE;
	}

	return 0;
}

/* va --> pa */
int mmap1page(u32 va, u32 pa, u32 usermode)
{
	/* get pde */
	u32 pde_idx = va >> 22, pte_idx = (va >> 12) & 0x3FF;
	pde_t *pde = (pde_t *)getCR3();
	pte_t *pte;
	if (pde[pde_idx].p == 0)
	{
		pte = (pte_t *)page_alloc(BUDDY_RANK_4K, MMAREA_NORMAL | PAF_ZERO);

		pde[pde_idx].pt_base = ((u32)pte) >> 12;
		if (usermode)
			pde[pde_idx].us = 1;
		else
			pde[pde_idx].us = 0;
		pde[pde_idx].rw = 1;
		pde[pde_idx].p = 1;
	}
	else
	{
		pte = (pte_t *)((pde[pde_idx].pt_base) << 12);
	}

	pte[pte_idx].pg_base = pa >> 12;
	if (usermode)
		pte[pte_idx].us = 1;
	else
		pte[pte_idx].us = 0;
	pte[pte_idx].rw = 1;
	pte[pte_idx].p = 1;

	return 0;
}
