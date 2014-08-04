#include "public.h"
#include "config.h"
#include "boot.h"

__asm__(".code16gcc\n");



/* causion: we just have 512B */

struct DiskAddressPacket {
	u8 PacketSize;				/* 16 byte, this struct size, fixed */
	u8 Reserved;
	u16 BlockCount;				/* sect num need to transform */
	 
	u32 BufferAddr;				/* (segment:offset) */
	u64 BlockNum;				/* start LBA */
}__attribute__((packed));


struct DiskAddressPacket volatile da =
{
	.PacketSize = sizeof(struct DiskAddressPacket),
	.Reserved = 0,
	.BlockNum = IMGCORE_OFFSET,
};

void bootc_entry () __attribute__ ((noreturn));

void bootc_entry(void)
{
	u32 coreaddr = IMGCORE_LOADADDR;
	u16 n_sect, cn;
	struct bootparam *bp = (struct bootparam *)(0x200 - sizeof(struct bootparam));


	/* save the boot_dev id, which is in dl now */
    __asm__ __volatile__ (
    	"mov	%%dl, %0		\n\t"
    	:"=m"(bp->boot_dev)
	);
	

    /* we want to load the core.bin
       addr: IMGCORE_LOADADDR_BASE:IMGCORE_LOADADDR_OFFSET */
	
	n_sect = bp->n_sect;
	while (n_sect)
	{
		if (n_sect > 32)
		{
			cn = 32;
		}
		else
		{
			cn = n_sect;
		}
		da.BlockCount = cn;
		da.BufferAddr = ((coreaddr & 0xFFFF0000) << 12) | (coreaddr & 0xFFFF);
		/* int 0x13 ah=0x42 */
		__asm__ __volatile__ (
			"pusha					\n\t"
			"mov	$0x42, %%ah		\n\t"
			"int	$0x13			\n\t"
			"popa					\n\t"
			:
			:"S"(&da), "d"(bp->boot_dev)
		);
		barrier();

		n_sect -= cn;
		coreaddr += cn * HD_SECTOR_SIZE;
		da.BlockNum += cn;
	}
	
	/* now coreaddr is useless,      (H16 bit)                 (L16 bit)
	   32bit store the entryaddr: IMGCORE_LOADADDR_BASE:IMGCORE_LOADADDR_OFFSET */
	// coreaddr = IMGCORE_LOADADDR;

    /* jump to the core entry */
    __asm__ __volatile__ (
    	"jmp	%0, %1					\n\t"
        :
        :"i"(IMGCORE_LOADADDR_BASE), "i"(IMGCORE_LOADADDR_OFFSET)
    );

	/* make compiler happy */
	while(1);
}

