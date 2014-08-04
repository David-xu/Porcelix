#include "public.h"
#include "typedef.h"
#include "assert.h"
#include "section.h"
#include "desc.h"
#include "io.h"
#include "hd.h"
#include "debug.h"
#include "hdreg.h"
#include "memory.h"
#include "interrupt.h"
#include "command.h"
#include "pci.h"
#include "module.h"
#include "task.h"
#include "boot.h"
#include "crc32.h"

struct bootparam bootparam _SECTION_(.coreentry.param);

void getbootparam()
{
    u32 i;
    ASSERT(sizeof(bootparam) == BOOTPARAM_PACKADDR_LEN);
    ASSERT(bootparam.bootsectflag == 0xAA55);

    for (i = 0; i < HD_PDTENTRY_NUM; i++)
    {
        hd0_pdt[i] = bootparam.hd_pdt[i];
    }
}

// void loader_entry(void) __attribute__((noreturn));

void loader_entry(void)
{
	/* we just check the loader buff */
	u32 corecrc = crc32((void *)(0x4000),
						bootparam.n_sect * HD_SECTOR_SIZE - 0x4000);

    /* copy the boot param from boot.bin */
    getbootparam();

    interrupt_init();		/* after that we enable INT */
    disp_init();            /* after that we can do some screen print... */
	
    /* check the crc */
	if (bootparam.core_crc != corecrc)
	{
		ERROR("CRC check failed. 0x%#8x != 0x%#8x, Total core sectors: %d\n",
			  bootparam.core_crc,
			  corecrc,
			  bootparam.n_sect);
	}
	else
	{
		printf("CRC chech success.\n");
	}

    /* call all the module init functions as the level order */
    init_module();

    idleloop();
}

