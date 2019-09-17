#include "public.h"
#include "config.h"
#include "boot.h"

asm(".code16gcc\n");



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
	// .BlockNum = IMGCORE_OFFSET,
};

void bootc_entry () __attribute__ ((noreturn));
#if 1
void bootc_entry(void)
{
	u32 coreaddr = IMGCORE_LOADADDR + 0x1000;
	u16 n_sect, cn;
	struct bootparam *bp = (struct bootparam *)(0x200 - sizeof(struct bootparam));
#if 0
	if (bp->boot_dev == 0x80)
		da.BlockNum = bp->core_lba;
	else
#endif
		da.BlockNum = bp->core_lba / 4;

    /* we want to load the core.bin
       addr: IMGCORE_LOADADDR_BASE:IMGCORE_LOADADDR_OFFSET */
	n_sect = bp->n_sect;
	while (n_sect)
	{
		if (n_sect > 4)
		{
			cn = 4;
		}
		else
		{
			cn = n_sect;
		}
#if 0
		if (bp->boot_dev == 0x80)
			da.BlockCount = cn;
		else
#endif
			da.BlockCount = (cn + 3) / 4;

		// da.BufferAddr = ((coreaddr & 0x000FFFF0) << 12) | (coreaddr & 0xF);
		da.BufferAddr = ((coreaddr & 0xFFFF0000) << 12) | (coreaddr & 0xFFFF);

		/* int 0x13 ah=0x42 */
		__asm__ __volatile__ (
			"pusha					\n\t"
			"mov	$0x42, %%ah		\n\t"
			"int	$0x13			\n\t"
			"popa					\n\t"
			:
			:"S"(&da), "d"(bp->boot_dev)
			:"%ax"			/* ax has been used, need to notify the gcc */
		);
		barrier();

		n_sect -= cn;
		coreaddr += cn * HD_SECTOR_SIZE;
#if 0
		if (bp->boot_dev == 0x80)
			da.BlockNum += cn;
		else
#endif
			da.BlockNum += cn / 4;
	}
	/* now coreaddr is useless,      (H16 bit)                 (L16 bit)
	   32bit store the entryaddr: IMGCORE_LOADADDR_BASE:IMGCORE_LOADADDR_OFFSET */
	// coreaddr = IMGCORE_LOADADDR;

    /* jump to the core entry
     * the macro CONFIG_CORE_ENTRYADDR defined by gcc compile param
	 */
    __asm__ __volatile__ (
    	"jmp	%0, %1					\n\t"
        :
        :"i"(IMGCORE_LOADADDR_BASE), "i"(IMGCORE_LOADADDR_OFFSET + CONFIG_CORE_ENTRYADDR)
    );

	/* make compiler happy */
	while(1);
}
#endif

