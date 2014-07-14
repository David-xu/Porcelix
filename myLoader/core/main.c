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

struct bootparam {
    u8  boot_dev;
    u8  rsv[29];
    struct hd_dptentry hd_pdt[4];
    u16 bootsectflag;           /* 0xAA55 */
}__attribute__((packed));

struct bootparam bootparam _SECTION_(.init.data);
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
    /* check the crc */
    
    /* copy the boot param from boot.bin */
    getbootparam();

    interrupt_init();
    disp_init();            /* after that we can do some screen print... */

    /* call all the module init functions as the level order */
    init_module();

    idleloop();
}

