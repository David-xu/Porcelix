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
#include "timer.h"

struct bootparam bootparam _SECTION_(.coreentry.param);

struct bootparam *g_bootp;

void getbootparam()
{
    ASSERT(sizeof(bootparam) == BOOTPARAM_PACKADDR_LEN);
    ASSERT(bootparam.bootsectflag == 0xAA55);

	g_bootp = &bootparam;
}

// void loader_entry(void) __attribute__((noreturn));

void loader_entry(void)
{
	/* we just check the loader buff */
	u32 corecrc = crc32((void *)(0x4000),
						bootparam.n_sect * HD_SECTOR_SIZE - 0x3000);
	bootparam.head4k_crc = crc32((void *)0, 0x1000);

    /* copy the boot param from boot.bin */
    getbootparam();

    interrupt_init();		/* after that we enable INT */
	trap_init();
    disp_init();            /* after that we can do some screen print... */
	mem_init();				/* we need to init all memsystem first */
	timer_init();

	printk("booting from 0x%#2x\n", bootparam.boot_dev);
	
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
		DEBUG("CRC chech success.\n");
	}

    /* call all the module init functions as the level order */
    init_module();

    idleloop();
}

