#include "public.h"
#include "config.h"
#include "boot.h"
#include "hd.h"
#include "memory.h"

asm(".code16gcc\n");


extern struct bootparam bootparam;
extern struct hd_info hd0_info;
extern struct hd_info hd1_info;

static u16 read16(u16 ds, u16 off)
{
	u16 ret;
	__asm__ __volatile__ (
		"push	%%ds			\n\t"
		"mov	%%ax, %%ds		\n\t"
		"mov	(%%bx), %%cx	\n\t"
		"pop	%%ds			\n\t"
		"mov	%%cx, %0		\n\t"
		:"=m"(ret)
		:"a"(ds), "b"(off)
		:"%cx"
	);

	return ret;
}


static void move_16(u32 dst, u32 src, u16 size)
{
	/* ds:si--->es:di */
	__asm__ __volatile__ (
		"pusha					\n\t"
		"push	%%ds			\n\t"
		"push	%%es			\n\t"
		"mov	%%dx, %%es		\n\t"
		"mov	%%ax, %%ds		\n\t"
		"rep	movsw			\n\t"
		"pop	%%es			\n\t"
		"pop	%%ds			\n\t"
		"popa					\n\t"
		:
		: "d"(dst >> 4), "a"(src >> 4), "c"(size >> 1),
		  "D"(dst & 0xF), "S"(src & 0xF)
		: "memory"
	);
}

/* just output any byte to port 0x80, we used it to delay */
static inline void io_delay_16(void)
{
	const u16 DELAY_PORT = 0x80;
	asm volatile("outb %%al,%0" : : "dN" (DELAY_PORT));
}

/* Basic port I/O */
static inline void outb_16(u8 v, u16 port)
{
	asm volatile("outb %0,%1" : : "a" (v), "dN" (port));
}
static inline u8 inb_16(u16 port)
{
	u8 v;
	asm volatile("inb %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

/* read until the 8042's buff get empty
   I just copy the code from the linux kernel :) */
#define MAX_8042_LOOPS__	100000
#define MAX_8042_FF__	32
static int _SECTION_(.init.text) empty_8042_16(void)
{
	u8 status;
	int loops = MAX_8042_LOOPS__;
	int ffs   = MAX_8042_FF__;

	while (loops--) {
		io_delay_16();

		status = inb_16(0x64);
		if (status == 0xff) {
			/* FF is a plausible, but very unlikely status */
			if (!--ffs)
				return -1; /* Assume no KBC present */
		}
		if (status & 1) {
			/* Read and discard input data */
			io_delay_16();
			(void)inb_16(0x60);
		} else if (!(status & 2)) {
			/* Buffers empty, finished! */
			return 0;
		}
	}

	return -1;
}


static u8 _SECTION_(.init.text) kbdctrl_outport_r_16(void)
{
    outb_16(0xD0, 0x64);
    io_delay_16();
    return inb_16(0x60);
}

static void _SECTION_(.init.text) kbdctrl_outport_w_16(u8 cmd)
{
	empty_8042_16();

    outb_16(0xD1, 0x64);

	empty_8042_16();

    outb_16(cmd, 0x60);
}

void _SECTION_(.init.text) disableA20_16()
{
	kbdctrl_outport_w_16(kbdctrl_outport_r_16() & 0xFD);
}

/* entry of the core.elf */
void _SECTION_(.init.text) corestart_16c(void)
{
	u32 coresize, cs;
	u32 dstaddr = 0x1000, srcaddr = IMGCORE_LOADADDR + 0x1000;

	/* step 1: get some info from boot.bin, the first 512B */
	move_16(IMGCORE_LOADADDR + (u32)(&bootparam),
			(BOOTPARAM_PACKADDR_BASS << 4) | BOOTPARAM_PACKADDR_OFFSET,
			BOOTPARAM_PACKADDR_LEN);

	/* step 2: get some info by BIOS */

	/* read the harddisc information, then store it in global var hd0_info */
	/* load the word in 0000:0106 to ds, load the word in 0000:0104 to si */
	/* ds:si -> es:di */
	move_16(IMGCORE_LOADADDR + (u32)(&hd0_info),
			(((u32)(read16(0, HD0_INFOADDR_ENTRYADDR_OFFSET + 2))) << 4) +
			read16(0, HD0_INFOADDR_ENTRYADDR_OFFSET),
			sizeof(hd0_info));
	/* read the harddisc information, then store it in global var hd1_info */
	/* load the word in 0000:011a to ds, load the word in 0000:0118 to si */
	/* ds:si -> es:di */
	move_16(IMGCORE_LOADADDR + (u32)(&hd1_info),
			(((u32)(read16(0, HD1_INFOADDR_ENTRYADDR_OFFSET + 2))) << 4) +
			read16(0, HD1_INFOADDR_ENTRYADDR_OFFSET),
			sizeof(hd1_info));

	/* step3. get the System RAM information */
	/* int 15 ax:0xE820, get the ram info and store in the ds:di */
	__asm__ __volatile__ (
		"pusha					\n\t"
		"push	%%ds			\n\t"
		"mov	%%ax, %%ds		\n\t"
		"mov	$0xE801, %%ax	\n\t"
		"int	$0x15			\n\t"
		"mov	%%ax, (%%di)	\n\t"
		"mov	%%bx, 2(%%di)	\n\t"
		"mov	%%cx, 4(%%di)	\n\t"
		"mov	%%dx, 6(%%di)	\n\t"
		"pop	%%ds			\n\t"
		"popa					\n\t"
		
		:
		:"a"((IMGCORE_LOADADDR + (u32)(&raminfo_buf)) >> 4),
		 "D"((IMGCORE_LOADADDR + (u32)(&raminfo_buf)) & 0xF)
		:"memory"
	);

	/* step4. backup current IDRT, it's the BIOS interrupt vect tbale */
	__asm__ __volatile__ (
		"sidt	%0				\n\t"
		:"=m"(bootparam.idtr_back)
		:
	);

	/* move the core from
	   IMGCORE_LOADADDR_BASE : IMGCORE_LOADADDR_OFFSET
	   to
	   0:0*/
	coresize = bootparam.n_sect * HD_SECTOR_SIZE;
	while (coresize)
	{
		if (coresize > 32 * 1024)
		{
			cs = 32 * 1024;
		}
		else
		{
			cs = coresize;
		}
		move_16(dstaddr, srcaddr, cs);

		dstaddr += cs;
		srcaddr += cs;
		coresize -= cs;
	}

	jump2pe();
}

