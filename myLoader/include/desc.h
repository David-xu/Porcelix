#ifndef _DISC_H_
#define _DISC_H_

#include "io.h"
#include "config.h"
#include "string.h"

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
    X86_VECTOR_IRQ_22,
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
    X86DESCTYPE_IDT_32 = 0xE,
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
    printf("high word: %x, low word: %x\n", desc->u.s1.word1, desc->u.s1.word0);
}

/******************************************************************************/
/******************************************************************************/
static inline void set_idt(enum X86_VECTORTYPE type, void *addr)
{
    struct idtdesc idt;

    idt.type = X86DESCTYPE_IDT_32;
    idt.funcaddr_h16 = ((u32)addr) >> 16;
    idt.funcaddr_l16 = (u16)(u32)addr;

    idt.dpl = 0;
    idt.p = 1;
    idt.s = 0;
    idt.desc = SYSDESC_CODE;

    memcpy((u8 *)&(idt_table[type]), (u8 *)&idt, sizeof(idt));
}

#endif

