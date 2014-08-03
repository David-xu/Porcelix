#include "public.h"
#include "config.h"
#include "boot.h"
#include "hd.h"
#include "memory.h"

__asm__(".code16gcc\n");

extern struct bootparam bootparam;
extern struct hd_info hd0_info;
extern struct hd_info hd1_info;

extern void jump2pe(void);


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


/* entry of the core.elf */
void _SECTION_(.init.text) corestart_16c(void)
{
	u32 coresize, cs;
	u32 dstaddr = 0, srcaddr = IMGCORE_LOADADDR;
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

