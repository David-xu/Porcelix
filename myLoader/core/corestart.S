#include "config.h"

/******************************************************************************/
/* bootsect end up, this is the start of the core.img                         */
/*                                                                            */
/******************************************************************************/
.section .init.start
.code16

/* change the mode to PE */
/* this is also the smp ap entry, it should be located align to 4K */
.global jump2pe

jump2pe:
cli

/* prepare the seg desc */
xorw	%ax, %ax
movw	%ax, %ds
lgdt	lgdt_value

/* switch to the 'protect' mode */

/* turn on the protect bit */
movl	$0x1, %eax
movl	%eax, %cr0

nop
nop

/* change into the protect mode */
ljmp	$SYSDESC_CODE, $seg_init

.global core_entry
core_entry:

mov		%cs, %ax
mov		%ax, %ds
mov		%ax, %es

.global corestart_16c
jmp		corestart_16c

swith2realmode_1:
movw	$REALMODE_DS, %ax
movw	%ax, %ds
movw	%ax, %es
movw	%ax, %ss
movw	%ax, %fs
movw	%ax, %gs
movl	$0xe000, %esp
.global disableA20_16
/* we need to add this 'operand-size override' prefix before this call, because the .c file compiled by .code16gcc, which the 0x66 add before the 'ret' */
calll	disableA20_16

/* disable protected mode */
movl	%cr0, %eax
andl	$0xFFFFFFFE, %eax
movl	%eax, %cr0

/* 5. prepare the register */
movw	$0x7000, %ax
movw	%ax, %ds
movw	%ax, %es
movw	%ax, %ss
movw	%ax, %fs
movw	%ax, %gs
movw	$0xe000, %sp

/* jump to the entry, this is only used for linux kernel */
jmp		$0x7020, $0x0

.code32
/* be careful, this entry should located at the address which below 64K (0x10000)
 * and it should be the 32bit code
 */
.global swith2realmode
swith2realmode:
ljmp	$REALMODE_CS, $swith2realmode_1




/**********************************/
/* now the system is in IA32 mode */
/*                                */
/*                                */
/**********************************/
.section .text

/* NOW we are in PE mode */
seg_init:

movw	$SYSDESC_DATA, %ax
movw	%ax, %ss
movw	%ax, %ds
movw	%ax, %es
movw	%ax, %fs
xorw	%ax, %ax
movw	%ax, %gs

/* init sp */
movl	$SYSINIT_SP, %eax
movl	%eax, %esp
movl	%eax, %ebp

/* prepare the intrrupt vectbl */
lidt	lidt_value

#ifndef CONFIG_SMP

call	loader_entry

#else
/* now the loader goes well, just jump to the main function of myLoader */

/* BSP will jmp to the loader_entry()
   AP's will jmp to the smp_apentry */
call	*(initial_code)

/* this is smp entry,  */
.global smp_apentry
.global smp_nap
smp_apentry:

	/* atomic add the number of ap */
	add	$1, smp_nap

smp_apentry_loop:
	movl	smpstart_addr, %ecx
	jcxz	smp_apentry_loop
	
/* let's check the APIC ID, IA32_APIC_BASE MSR: addr->0x1b */
	movl	$0xFEE00020, %ebx
	movl	(%ebx), %eax
	shr		$22, %eax
	movl	smpap_initsp(%eax), %esp
	call	*smpstart_addr

#endif

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


/* code segment desc */
/*
 * BASE  : 0x0
 * LIMIT : 
 */
.section .gdt.data
SYS_SEGDESC:
.quad	0x0000000000000000
.quad	0x0000000000000000
.quad	0x00cf9a000000ffff			/* RING0 CS */
.quad	0x00cf92000000ffff			/* RING0 DS */
.quad	0x00cffa000000ffff			/* RING3 CS */
.quad	0x00cff2000000ffff			/* RING3 DS */
.quad	0x0							/* TSS */
.quad	0x00009b000000ffff			/* REALMODE CS */
.quad	0x000093000000ffff			/* REALMODE DS */
.fill	7, 8, 0						/* let's make some slot */

.section .idt.data
INT_SEGDESC:
.fill	256, 8, 0

.section .init.data
lgdt_value:
.word	16 * 8 - 1
.long	SYS_SEGDESC

lidt_value:
.word	256 * 8 - 1
.long	INT_SEGDESC

#ifdef CONFIG_SMP
.align 4
.global initial_code
initial_code:
.long	loader_entry

/* BSP will set this flag until all AP's task stack has been prepared */
.global smpstart_addr
smpstart_addr:
.long	0

.global smpap_initsp
smpap_initsp:
.fill	16, 4, 0
#endif

