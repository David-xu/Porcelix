#ifndef _IO_H_
#define _IO_H_

#include "typedef.h"
#include "section.h"

/* just output any byte to port 0x80, we used it to delay */
static inline void io_delay(void)
{
	const u16 DELAY_PORT = 0x80;
	asm volatile("outb %%al,%0" : : "dN" (DELAY_PORT));
}

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
    char *name;
    void (*init)(struct console *con);
    void (*write)(struct console *con, char *buf, u16 len);
    u16 (*read)(struct console *con, char *buf, u16 len);
    
    void *param;

    void (*spcdisp)(struct console *con, char *buf, u16 len, u8 swch);
	u8	spcdisp_swch;				/* 1: switch on, 0: switch off */
	char spcbuff[32];				/* line 0, right hand */
};

void disp_init() _SECTION_(.init.text);

int printf(char *fmt, ...);
int conspc_printf(char *fmt, ...);
void conspc_off(void);
int sprintf(char *buf, char *fmt, ...);
int vsprintf(char *buf, char *fmt, u32 *args);

/* kbd get one char */
int kbd_get_char();
u8 kbdctrl_outport_r(void);
void kbdctrl_outport_w(u8 cmd);

#endif


