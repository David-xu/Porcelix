#include "config.h"

/******************************************************************************/
/* bootsect end up, this is the start of the core.img                         */
/*                                                                            */
/******************************************************************************/
.section .init.start
.code16

/* get some info by BIOS */

/* read the hd partition table, then store it in global var hd0_pdt */
.global hd0_pdt
/* ds:si -> es:di */
movw	$HD_PDTADDR_BASS, %ax
movw	%ax, %ds
movw	$HD_PDTADDR_OFFSET, %si
movw	$(IMGCORE_LOADADDR_BASE + IMGCORE_LOADADDR_OFFSET >> 4), %ax	/* prepare es and di */
movw	%ax, %es
movw	$hd0_pdt, %di
movw	$32, %cx
rep		movsw

/* read the harddisc information, then store it in global var hd0_info */
.global hd0_info
/* load the 0000:0106 ---> ds, 0000:0104 ---> si */
/* ds:si -> es:di */
movw	$HD_INFOADDR_ENTRYADDR_BASE, %ax
movw	%ax, %ds
movw	$HD_INFOADDR_ENTRYADDR_OFFSET, %di
movw	2(%di), %ax		/* ---> ds */
movw	(%di), %bx		/* ---> si*/
movw	%ax, %ds
movw	%bx, %si
movw	$(IMGCORE_LOADADDR_BASE + IMGCORE_LOADADDR_OFFSET >> 4), %ax	/* prepare es and di */
movw	%ax, %es
movw	$hd0_info, %di
movw	$8, %cx
rep		movsw

/* get the System RAM information */
.global raminfo_buf
movw	$(IMGCORE_LOADADDR_BASE + IMGCORE_LOADADDR_OFFSET >> 4), %ax	/* prepare es and di */
movw	%ax, %es
movw	%ax, %ds
movw	$raminfo_buf, %di

/* int 15 ax:0xE820, get the ram info and store in the es:di */
movw	$0, %bx
movw	$0xE801, %ax
int		$0x15
movw	%ax, (%di)
movw	%bx, 2(%di)
movw	%cx, 4(%di)
movw	%dx, 6(%di)






/******************************************************************************
 * now we prepare to mov the whole system to zero address and runnnnnnnn.....  *
 *                                                                            *
 ******************************************************************************/
/* mov myself to 0:0 */
movw	$IMGCORE_SIZE_WORD, %cx

/* rep: ds:si -> es:di */
movw	$(IMGCORE_LOADADDR_BASE + IMGCORE_LOADADDR_OFFSET >> 4), %ax
movw	%ax, %ds
xor		%ax, %ax
movw	%ax, %si	/* si == 0x0 */


movw	%ax, %es	/* es == 0x0 */
movw	%ax, %di	/* di == 0x0 */

rep		movsw




/* change the mode to PE */

/* prepare the seg desc */
xorw	%ax, %ax
movw	%ax, %ds
lgdt	lgdt_value



/* switch to the 'protect' mode */

/* turn on the protect bit */
movl	$0x1, %eax
movl	%eax, %cr0

/* change into the protect mode */
ljmp	$SYSDESC_CODE, $seg_init




/**********************************/
/* now the system is in IA32 mode */
/*                                */
/*                                */
/**********************************/
.section .text
.code32

seg_init:

movw	$SYSDESC_DATA, %ax
movw	%ax, %ss
movw	%ax, %ds
movw	%ax, %es

/* init sp */
movl	$SYSINIT_SP, %eax
movl	%eax, %esp

/* prepare the intrrupt vectbl */
lidt	lidt_value

/* now the loader goes well, just jump to the main function of myLoader */
ljmp	$SYSDESC_CODE, $loader_entry


/*
 * This is the Global Descriptor Table
 *
 *  An entry, a "Segment Descriptor", looks like this:
 *
 * 31          24         19   16                 7           0
 * ------------------------------------------------------------
 * |             | |B| |A|       | |   |1|0|E|W|A|            |
 * | BASE 31..24 |G|/|L|V| LIMIT |P|DPL|  TYPE   | BASE 23:16 |  4
 * |             | |D| |L| 19..16| |   |1|1|C|R|A|            |
 * ------------------------------------------------------------
 * |                             |                            |
 * |        BASE 15..0           |       LIMIT 15..0          |  0
 * |                             |                            |
 * ------------------------------------------------------------
 *
 *  Note the ordering of the data items is reversed from the above
 *  description.
 */

.section .gdt.data
/* code segment desc */
/*
 * BASE  : 0x0
 * LIMIT : 
 */
.align 8
SYS_SEGDESC:
.quad	0x0000000000000000
.quad	0x0000000000000000
.quad	0x00cf9a000000ffff
.quad	0x00cf92000000ffff

.section .idt.data
INT_SEGDESC:
.fill	256, 8, 0

.section .init.data
lgdt_value:
.word	4 * 8 - 1
.long	SYS_SEGDESC

lidt_value:
.word	256 * 8 - 1
.long	INT_SEGDESC