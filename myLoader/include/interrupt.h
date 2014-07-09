#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include "section.h"
#include "list.h"

#define _cli()      do {asm volatile("cli":::"memory");} while (0)
#define _sti()      do {asm volatile("sti":::"memory");} while (0)

static inline unsigned long native_save_fl(void)
{
	unsigned long flags;

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

static inline void native_restore_fl(unsigned long flags)
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


typedef void (*intproc_ent)(void *param);

typedef struct interrupt_proc {
    struct list_head int_list;          /* interrupt list */
    intproc_ent intf;
    void *param;
} interrupt_proc_t;

extern void int10_public_entrance(void);
extern void kbd_int_entrance(void);
extern void hd_int_entrance(void);

extern void lapictimer_entrance(void);


void interrupt_init() _SECTION_(.init.text);

int interrup_register(int irq, intproc_ent intf, void *param);

#endif

