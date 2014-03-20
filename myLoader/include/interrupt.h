#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include "section.h"

#define _cli()      {asm volatile("cli"::);}
#define _sti()      {asm volatile("sti"::);}


#define PORT_8259A_MASTER_A0            (0x20)
#define PORT_8259A_MASTER_A1            (0x21)
#define PORT_8259A_SLAVER_A0            (0xA0)
#define PORT_8259A_SLAVER_A1            (0xA1)


void interrupt_init() _SECTION_(.init.text);

extern void kbd_int_entrance(void);
extern void hd_int_entrance(void);



#endif

