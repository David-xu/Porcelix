#ifndef _IO_H_
#define _IO_H_

#include "typedef.h"
#include "section.h"

/* Basic port I/O */
static inline void outb(u8 v, u16 port)
{
	asm volatile("outb %0,%1" : : "a" (v), "dN" (port));
}
static inline u8 inb(u16 port)
{
	u8 v;
	asm volatile("inb %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

static inline void outw(u16 v, u16 port)
{
	asm volatile("outw %0,%1" : : "a" (v), "dN" (port));
}
static inline u16 inw(u16 port)
{
	u16 v;
	asm volatile("inw %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

static inline void outl(u32 v, u16 port)
{
	asm volatile("outl %0,%1" : : "a" (v), "dN" (port));
}
static inline u32 inl(u16 port)
{
	u32 v;
	asm volatile("inl %1,%0" : "=a" (v) : "dN" (port));
	return v;
}



struct console{
    u8 *name;
    void (*init)(struct console *con);
    void (*write)(struct console *con, u8 *buf, u16 len);
    u16 (*read)(struct console *con, u8 *buf, u16 len);     // 
    
    void *param;
};

void disp_init() _SECTION_(.init.text);
void kbd_init() _SECTION_(.init.text);
void hd_init() _SECTION_(.init.text);


int printf(u8 *fmt, ...);
int sprintf(u8 *buf, u8 *fmt, ...);


/* kbd get one char */
int kbd_get_char();
u8 kbdctrl_outport_r(void);
void kbdctrl_outport_w(u8 cmd);

#endif


