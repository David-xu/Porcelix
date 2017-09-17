/* the base address of .text is 0x7c00 */

#include "config.h"

.global bootc_entry
.global boot_dev

.section .text.entry

/* x86 read mode */
.code16

/*  */
#if 0
test_flag:
	movw	$0x9000, %ax
	movw	%ax, %sp
	push	$0x7c00
.byte 0x66
	ret
#endif
/*  */

	cli
/* init ss:sp take care of this value */
	movw	$0x9000, %ax
	movw	%ax, %ss
	movw	$0x0, %sp

	movw	$0x7C0, %ax
	movw	%ax, %ds
	movw	%ax, %es

	sti		/* we're safe again */

	/* save the boot device */
	movb	%dl, boot_dev
	// movb	%dl, %al
	// call	print_hex
	/* goto main boot entry */
	jmp	$0x7c0, $bootc_entry

	/* test: int0x13 ah0x42 */
#if 0
	pushw	$0x0010
	pushw	$0x1
	/* buff addr seg:off */
	pushw	%ss
	pushw	$0
	/* start lba */
	pushw	$0
	pushw	$0
	pushw	$0
	pushw	$0x22
	movw	%ss, %ax
	movw	%ax, %ds
	movw	%sp, %si
2:
	movw	$0x42, %ax
	int		$0x13

	pushf
	popw	%ax
	call	print_ch
	
	jc		2b
	
3:
	jmp		3b
	movb	%dl, %al
	shrw	$0x4, %ax
	call	print_ch

	movb	$0xc, %al
	call	print_ch
#endif
/*
 * print char
 * al: char
 */
.global print_hex
print_hex:
	pushw	%ax
	shrw	$4, %ax
	call	print_hex_
	popw	%ax
	call	print_hex_
	ret

print_hex_:
	pusha
	pushw	%es
	andw	$0xf, %ax
	movw	$0xa, %bx
	cmpw	%bx, %ax
	jb		print_hex_ba
	addw	$0x7, %ax
print_hex_ba:
	addw	$0x30, %ax
	pushw	%ax
	movw	%sp, %bp
	movw	%ss, %ax
	movw	%ax, %es
	movw	$0x1301, %ax
	movw	$0x0002, %bx
	movw	$0x1, %cx
	
	movw	print_pos_x, %dx
	addw	$1, %dx
	movw	%dx, print_pos_x
	
	int		$0x10
	popw	%ax
	popw	%ax
	movw	%ax, %es
	popa
	ret

print_pos_x:
	.word	0
	




#if 0
struct    DiskAddressPacket {
BYTE PacketSize;		// 数据包尺寸(16字节)
BYTE Reserved;			// == 0
WORD BlockCount;		// 要传输的数据块个数(以扇区为单位)
 
DWORD BufferAddr;		// 传输缓冲地址(segment:offset)
QWORD BlockNum;			// 磁盘起始绝对块地址
};

dick_address_pack:
	.byte	16
	.byte	0
	.word	SECTORNUM_CORESIZE
	.long	IMGCORE_LOADADDR
	.quad	IMGCORE_OFFSET


core_loader:
	/* load the core IMGCORE_LOADADDR_BASE:IMGCORE_LOADADDR_OFFSET */
	movw	$dick_address_pack, %si
	movb	$0x42, %ah				/* disk block read */
	movb	%dl, %dl				/* use the boot device as the img device */
	int		$0x13

/* jump to the core entry */
	jmp	$IMGCORE_LOADADDR_BASE, $IMGCORE_LOADADDR_OFFSET

.org	0x1A0
n_sect:		.word 0			/*  */
boot_dev:	.byte 0			/*  */

.org	0x1FE
.word	0xAA55			/* off:0x1FE */

#endif
