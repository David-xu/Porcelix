#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include "section.h"
#include "list.h"
#include "typedef.h"
#include "io.h"
#include "desc.h"

struct pt_regs {
	long ebx;
	long ecx;
	long edx;
	long esi;
	long edi;
	long ebp;
	long eax;
	int  xds;
	int  xes;
	int  xfs;				
	int  xgs;				/* 0x28 */
	long orig_eax;			/* 0x2C */
	long eip;				/* 0x30 */
	int  xcs;				/* 0x34 */
	long eflags;			/* 0x38 */
	long esp;				/* 0x3C */
	int  xss;				/* 0x40 */
};

static inline void dump_eflags(u32 eflags)
{
	printk("eflags: 0x%#8x\n%s %s %s %s %s %s %s IOPL:%d %s %s %s %s %s %s %s %s %s\n",
		   eflags,
		   (eflags & EFLAGSMASK_ID) ? "ID" : "id",
		   (eflags & EFLAGSMASK_VIP) ? "VIP" : "vip",
		   (eflags & EFLAGSMASK_VIF) ? "VIF" : "vif",
		   (eflags & EFLAGSMASK_AC) ? "AC" : "ac",
		   (eflags & EFLAGSMASK_VM) ? "VM" : "vm",
		   (eflags & EFLAGSMASK_RF) ? "RF" : "rf",
		   (eflags & EFLAGSMASK_NT) ? "NT" : "nt",
		   (eflags & EFLAGSMASK_IOPL) >> 12,
		   (eflags & EFLAGSMASK_OF) ? "OF" : "of",
		   (eflags & EFLAGSMASK_DF) ? "DF" : "df",
		   (eflags & EFLAGSMASK_IF) ? "IF" : "if",
		   (eflags & EFLAGSMASK_TF) ? "TF" : "tf",
		   (eflags & EFLAGSMASK_SF) ? "SF" : "sf",
		   (eflags & EFLAGSMASK_ZF) ? "ZF" : "zf",
		   (eflags & EFLAGSMASK_AF) ? "AF" : "af",
		   (eflags & EFLAGSMASK_PF) ? "PF" : "pf",
		   (eflags & EFLAGSMASK_CF) ? "CF" : "cf");
}

static inline void dump_ptregs(struct pt_regs *regs)
{
	u32 cur_cs, cur_ss, cur_esp;
	__asm__ __volatile__ (
		"movl	%%cs, %%eax			\n\t"
		"movl	%%eax, %0			\n\t"
		"movl	%%ss, %%eax			\n\t"
		"movl	%%eax, %1			\n\t"
		"movl	%%esp, %2			\n\t"
		:"=m"(cur_cs), "=m"(cur_ss), "=m"(cur_esp)
		:
		:"%eax"
	);
	printk("\n\n"
		   "ebx:0x%#8x, ecx:0x%#8x, edx:0x%#8x\n"
		   "esi:0x%#8x, edi:0x%#8x, ebp:0x%#8x\n"
		   " ds:0x%#4x,      es:0x%#4x,      fs:0x%#4x,  gs:0x%#8x(entry function)\n"
		   "eip:0x%#8x,  cs:0x%#4x,  osp:0x%#8x, oss:0x%#4x, orig_eax:0x%#8x\n",
		   regs->ebx, regs->ecx, regs->edx,
		   regs->esi, regs->edi, regs->ebp,
		   (u16)(regs->xds), (u16)(regs->xes), (u16)(regs->xfs), regs->xgs,		/* this is the entry function addr */
		   regs->eip, (u16)(regs->xcs), regs->esp, regs->xss, regs->orig_eax);

	dump_eflags(regs->eflags);

	if ((cur_cs & 0x3) == ((regs->xcs) & 0x3))
	{
		printk("current ss:0x%#4x, esp:0x%#8x\n", (u16)cur_ss, cur_esp);
	}
}

#define _cli()      do {asm volatile("cli":::"memory");} while (0)
#define _sti()      do {asm volatile("sti":::"memory");} while (0)

static inline u32 native_save_fl(void)
{
	u32 flags;

	/*
	 * "=rm" is safe here, because "pop" adjusts the stack before
	 * it evaluates its effective address -- this is part of the
	 * documented behavior of the "pop" instruction.
	 */
	asm volatile("# __raw_save_flags\n\t"
		     "pushf ; pop %0"
		     : "=rm" (flags)
		     : /* no input */
		     : "memory");

	return flags;
}

static inline void native_restore_fl(u32 flags)
{
	asm volatile("push %0 ; popf"
		     : /* no output */
		     :"g" (flags)
		     :"memory", "cc");
}

#define raw_local_irq_save(flags)           \
	do {                                    \
        flags = native_save_fl();           \
        _cli();                             \
	} while (0)
#define raw_local_irq_restore(flags)        \
	do {                                    \
		native_restore_fl(flags);           \
	} while (0)


#define PORT_8259A_MASTER_A0            (0x20)
#define PORT_8259A_MASTER_A1            (0x21)
#define PORT_8259A_SLAVER_A0            (0xA0)
#define PORT_8259A_SLAVER_A1            (0xA1)

#define	INTPROC_DEFINE(vect)				\
	extern void intproc_##vect(void);

typedef void (*intproc_ent)(struct pt_regs *regs, void *param);

typedef struct int_proc {
    struct list_head int_list;          /* interrupt list */
    intproc_ent intf;
    void *param;
} int_proc_t;

void lapictimer_entrance(void);

void interrupt_init();

int interrup_register(int irq, intproc_ent intf, void *param);

void trap_init(void);


#endif

