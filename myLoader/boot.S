/* the base address of .text is 0x7c00 */

#include "config.h"

.text

/* x86 read mode */
.code16
	cli
	movw	$0x7C0, %ax
	movw	%ax, %ds
	movw	%ax, %ss

/* init ss:sp take care of this value */
	movw	$0x400, %sp

	sti		/* we're safe again */

	jmp	core_loader

/*
struct    DiskAddressPacket {
BYTE PacketSize;		// 数据包尺寸(16字节)
BYTE Reserved;			// == 0
WORD BlockCount;		// 要传输的数据块个数(以扇区为单位)
 
DWORD BufferAddr;		// 传输缓冲地址(segment:offset)
QWORD BlockNum;			// 磁盘起始绝对块地址
};
*/
dick_address_pack:
	.byte	16
	.byte	0
	.word	SECTORNUM_CORESIZE
	.long	IMGCORE_LOADADDR
	.quad	IMGCORE_OFFSET


core_loader:
	/* save the boot device */
	movb	%dl, boot_dev

	/* load the core IMGCORE_LOADADDR_BASE:IMGCORE_LOADADDR_OFFSET */
	movw	$dick_address_pack, %si
	movb	$0x42, %ah				/* disk block read */
	movb	%dl, %dl				/* use the boot device as the img device */
	int		$0x13


	

/* jump to the core entry */
	jmp	$IMGCORE_LOADADDR_BASE, $IMGCORE_LOADADDR_OFFSET

.org	0x1A0
boot_dev: .byte 0		/*  */

.org	0x1FE
.word	0xAA55			/* off:0x1FE */
