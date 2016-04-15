#ifndef _DISC_H_
#define _DISC_H_

#include "public.h"
#include "io.h"
#include "config.h"
#include "ml_string.h"

#define	EFLAGSMASK_CF				(0x1 << 0)
#define	EFLAGSMASK_PF				(0x1 << 2)
#define	EFLAGSMASK_AF				(0x1 << 4)
#define	EFLAGSMASK_ZF				(0x1 << 6)
#define	EFLAGSMASK_SF				(0x1 << 7)
#define	EFLAGSMASK_TF				(0x1 << 8)
#define	EFLAGSMASK_IF				(0x1 << 9)
#define	EFLAGSMASK_DF				(0x1 << 10)
#define	EFLAGSMASK_OF				(0x1 << 11)
#define	EFLAGSMASK_IOPL				(0x3 << 12)
#define	EFLAGSMASK_NT				(0x1 << 14)
#define	EFLAGSMASK_RF				(0x1 << 16)
#define	EFLAGSMASK_VM				(0x1 << 17)
#define	EFLAGSMASK_AC				(0x1 << 18)
#define	EFLAGSMASK_VIF				(0x1 << 19)
#define	EFLAGSMASK_VIP				(0x1 << 20)
#define	EFLAGSMASK_ID				(0x1 << 21)

enum X86_VECTORTYPE{
    X86_VECTOR_DE = 0,          /* 0 : divi error */
    X86_VECTOR_DB,              /* 1 : debug */
    X86_VECTOR_NMI,
    X86_VECTOR_BP,              /* 3 : break point */
    X86_VECTOR_OF,              /* 4 : overflow */
    X86_VECTOR_BR,              /* 5 :  */
    X86_VECTOR_UD,              /* 6 : undefined op */
    X86_VECTOR_NM,              /* 7 : no machine x87 */
    X86_VECTOR_DF,              /* 8 : */
    X86_VECTOR_,
    X86_VECTOR_TS,              /* 10 :  */
    X86_VECTOR_NP,              /* 11 :  */
    X86_VECTOR_SS,              /* 12 :  */
    X86_VECTOR_GP,              /* 13 :  */
    X86_VECTOR_PF,              /* 14 : page fault */
    X86_VECTOR_RSV,             /* 15 : reserve */
    X86_VECTOR_MF,              /* 16 :  */
    X86_VECTOR_AC,              /* 17 :  */
    X86_VECTOR_MC,              /* 18 :  */
    X86_VECTOR_XF,              /* 19 :  */

/* 8259A master */
    X86_VECTOR_IRQ_20 = 0x20,   /* 8253                     */
    X86_VECTOR_IRQ_21,          /* kbd INT                  */
    X86_VECTOR_IRQ_22,          /* the 8259A slaver         */
    X86_VECTOR_IRQ_23,          /* SeriPort 02              */
    X86_VECTOR_IRQ_24,          /* SeriPort 01              */
    X86_VECTOR_IRQ_25,          /* ParaPort 02              */
    X86_VECTOR_IRQ_26,          /* softdisk INT             */
    X86_VECTOR_IRQ_27,          /* ParaPort 01              */
/* 8259A slaver */
    X86_VECTOR_IRQ_28,          /* RealTimer INT            */
    X86_VECTOR_IRQ_29,          /* Reserve 01               */
    X86_VECTOR_IRQ_2A,          /* Reserve 02               */
    X86_VECTOR_IRQ_2B,          /* Reserve 03, network INT  */
    X86_VECTOR_IRQ_2C,          /* PS/2 mouse INT           */
    X86_VECTOR_IRQ_2D,          /* Math CoProcessor         */
    X86_VECTOR_IRQ_2E,          /* HardDisk INT             */
    X86_VECTOR_IRQ_2F,          /* Reserve 04               */

/* custom vector */
    CUSTOM_VECTOR_LAPICTIMER = 0x50,

/* system call vector */
	SYSTEMCALL_VECTOR = 0x80,
};

struct segdesc{
    union
    {
        struct
        {
            u32 word0;          /* low word */
            u32 word1;          /* high word */
        }s1;
        
        struct
        {
            /* word 0 */
            u32 seg_limit_l16 : 16;
            u32 base_addr_l16 : 16;
            /* word 1 */
            u32 base_addr_m8  : 8;
            u32 type          : 4;
            u32 sysflag       : 1;
            u32 dpl           : 2;
            u32 persist       : 1;
            u32 seg_limit_h4  : 4;
            u32 avl           : 1;
            u32 l             : 1;
            u32 b_d           : 1;
            u32 g             : 1;
            u32 base_addr_h8  : 8;
        }s2;
    }u;
};

enum desc_type
{
	X86DESCTYPE_TSS_32 = 0x9,
    X86DESCTYPE_IDT_32 = 0xE,
    X86DESCTYPE_TRAP_32 = 0xF,
};

struct idtdesc
{
    /* word0 */
    u32 funcaddr_l16        : 16;
    u32 desc                : 16;
    /* word1 */
    u32 recv                : 8;
    u32 type                : 4;
    u32 s                   : 1;
    u32 dpl                 : 2;
    u32 p                   : 1;
    u32 funcaddr_h16        : 16;
};

/* linker may set these symbols */
extern struct segdesc   gdt_table[];
extern struct idtdesc   idt_table[];


static inline void segdesc_disp(struct segdesc *desc)
{
/*
    u32 baseaddr = (desc->base_addr_h8 << 24) |
                   (desc->base_addr_m8 << 16) |
                   (desc->base_addr_l16);
    u32 limit = (desc->seg_limit_h4 << 16) |
                (desc->seg_limit_l16);
*/
    printk("high word: %x, low word: %x\n", desc->u.s1.word1, desc->u.s1.word0);
}

/******************************************************************************/
/******************************************************************************/
static inline void set_intgate(u8 vect, void *addr)
{
    struct idtdesc idt;

    idt.type = X86DESCTYPE_IDT_32;
    idt.funcaddr_h16 = ((u32)addr) >> 16;
    idt.funcaddr_l16 = (u16)(u32)addr;

    idt.dpl = 0;
    idt.p = 1;
    idt.s = 0;
    idt.desc = SYSDESC_CODE;

    memcpy((u8 *)&(idt_table[vect]), (u8 *)&idt, sizeof(idt));
}

static inline void set_trap(u8 vect, void *addr)
{
    struct idtdesc idt;

    idt.type = X86DESCTYPE_TRAP_32;
    idt.funcaddr_h16 = ((u32)addr) >> 16;
    idt.funcaddr_l16 = (u16)(u32)addr;

    idt.dpl = 3;
    idt.p = 1;
    idt.s = 0;
    idt.desc = SYSDESC_CODE;

    memcpy((u8 *)&(idt_table[vect]), (u8 *)&idt, sizeof(idt));
}

/* we used the 'PUBDESC_TSS' as TSS */
static inline void set_tss(void *addr, u32 segsize)
{
    struct segdesc tss;
	ASSERT(sizeof(tss) == 8);

	tss.u.s1.word0 = 0;
	tss.u.s1.word1 = 0;

	barrier();

	tss.u.s2.type = X86DESCTYPE_TSS_32;
	tss.u.s2.base_addr_h8 = (u8)(((u32)addr) >> 24);
	tss.u.s2.base_addr_m8 = (u8)(((u32)addr) >> 16);
	tss.u.s2.base_addr_l16 = (u16)(u32)addr;

	tss.u.s2.seg_limit_h4 = 0;
	tss.u.s2.seg_limit_l16 = segsize - 1;
	
	tss.u.s2.persist = 1;
	tss.u.s2.sysflag = 0;

    memcpy((u8 *)&(gdt_table[PUBDESC_TSS >> 3]), (u8 *)&tss, sizeof(tss));
}

static inline void set_tr(void)
{
	__asm__ __volatile__ (
		"ltr %w0"
		:
		:"q" (PUBDESC_TSS)
	);
}

#endif

