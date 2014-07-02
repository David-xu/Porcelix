#include "typedef.h"
#include "memory.h"
#include "debug.h"
#include "assert.h"
#include "command.h"
#include "debug.h"
#include "ml_string.h"
#include "module.h"

static void cmd_ramdisp_opfunc(char *argv[], int argc, void *param)
{
    static void *dispaddr = NULL;
    static u32 len = 1;
    if (argc == 1)
    {
        dump_ram(dispaddr, len);
        dispaddr = (void *)(((unsigned)dispaddr) + len);
    }
    else if (argc == 2) // ramdisp addr
    {
        dispaddr = (void *)str2num(argv[1]);
        dump_ram(dispaddr, len);
    }
    else if (argc == 3) // ramdisp addr len
    {
        dispaddr = (void *)str2num(argv[1]);
        len = str2num(argv[2]);
        dump_ram(dispaddr, len);
    }
    else
    {
        printf("Wrong command syntax, you should input \'ramdisp [addr] [len]\'.\n");
    }
}

struct command cmd_ramdisp _SECTION_(.array.cmd) =
{
    .cmd_name   = "ramdisp",
    .info       = "Diplay RAM data.",
    .op_func    = cmd_ramdisp_opfunc,
};

static void cmd_iorw_opfunc(char *argv[], int argc, void *param)
{
    u32 addr, val;
    if ((argc < 3) || (argc > 4))
    {
        goto invalidparam;
    }
    else if (argc == 3)
    {
        if (memcmp("-rb", argv[1], sizeof("-rb")) == 0)
        {
            addr = str2num(argv[2]);
            val = inb((u16)addr);
            printf("Read byte from I/O 0x%#4x:0x%#2x.\n", (u16)addr, (u8)val);
        }
        else if (memcmp("-rw", argv[1], sizeof("-rw")) == 0)
        {
            addr = str2num(argv[2]);
            val = inw((u16)addr);
            printf("Read word from I/O 0x%#4x:0x%#4x.\n", (u16)addr, (u16)val);
        }
        else if (memcmp("-rl", argv[1], sizeof("-rl")) == 0)
        {
            addr = str2num(argv[2]);
            val = inl((u16)addr);
            printf("Read long from I/O 0x%#4x:0x%#8x.\n", (u16)addr, val);
        }
        else
        {
            goto invalidparam;
        }
    }
    else if (argc == 4)
    {
        if (memcmp("-wb", argv[1], sizeof("-wb")) == 0)
        {
            addr = str2num(argv[2]);
            val = str2num(argv[3]);

            outb((u8)val, (u16)addr);
        }
        else if (memcmp("-ww", argv[1], sizeof("-ww")) == 0)
        {
            addr = str2num(argv[2]);
            val = str2num(argv[3]);

            outw((u16)val, (u16)addr);
        }
        else if (memcmp("-wl", argv[1], sizeof("-wl")) == 0)
        {
            addr = str2num(argv[2]);
            val = str2num(argv[3]);

            outl(val, (u16)addr);
        }
        else
        {
            goto invalidparam;
        }

    }
    else
    {
        goto invalidparam;
    }
    return;

invalidparam:
    printf("-r[b][w][l] [addr]      : Read I/O value from addr.\n"
           "-w[b][w][l] [addr] [val]: write I/O value to addr.\n");
}

struct command cmd_iorw _SECTION_(.array.cmd) =
{
    .cmd_name   = "iorw",
    .info       = "I/O Read Write.-r [addr], -w [addr] [val]",
    .op_func    = cmd_iorw_opfunc,
};


/*
 we call the int 0x15 (ax = 0xE801), get some ram info
 we save the ax, bx, cx, dx.
--------b-15E801-----------------------------
INT 15 - Phoenix BIOS v4.0 - GET MEMORY SIZE FOR >64M CONFIGURATIONS
	AX = E801h
Return: CF clear if successful
	    AX = extended memory between 1M and 16M, in K (max 3C00h = 15MB)
	    BX = extended memory above 16M, in 64K blocks
	    CX = configured memory 1M to 16M, in K
	    DX = configured memory above 16M, in 64K blocks
	CF set on error
Notes:	supported by the A03 level (6/14/94) and later XPS P90 BIOSes, as well
	  as the Compaq Contura, 3/8/93 DESKPRO/i, and 7/26/93 LTE Lite 386 ROM
	  BIOS
	supported by AMI BIOSes dated 8/23/94 or later
	on some systems, the BIOS returns AX=BX=0000h; in this case, use CX
	  and DX instead of AX and BX
	this interface is used by Windows NT 3.1, OS/2 v2.11/2.20, and is
	  used as a fall-back by newer versions if AX=E820h is not supported
	this function is not used by MS-DOS 6.0 HIMEM.SYS when an EISA machine
	  (for example with parameter /EISA) (see also MEM F000h:FFD9h), or no
	  Compaq machine was detected, or parameter /NOABOVE16 was given.
 */
struct raminfo{
    u16     ax, bx, cx, dx;
};
u16 raminfo_buf[RAMINFO_MAXLEN];

u32 ram_size;           /* we call the bios to get this param. */

static void a20enable_test(void) _SECTION_(.init.text);
static void a20enable_test(void)
{
    u32 *p1 = (u32 *)0x100000, *p2 = (u32 *)0x0, v1, v2;
    v1 = *p1;
    v2 = *p2;
    DEBUG("addr 0x%#8x ---> 0x%#8x, addr 0x%#8x ---> 0x%#8x.\n",
           (u32)p1, v1,
           (u32)p2, v2);
    *p1 = 0x5500ffaa;

    v1 = *p1;
    v2 = *p2;
    DEBUG("addr 0x%#8x ---> 0x%#8x, addr 0x%#8x ---> 0x%#8x.\n",
           (u32)p1, v1,
           (u32)p2, v2);
    if (v1 == v2)
    {
        printf("a20 test faild.\nDead...");
        while (1);
    }
    printf("a20 test success.\n");
}


/******************************************************************************
 *               (1)                (2)
 *   (.init.text) |     system       |   hole   |   page_list   |
 *  (1). value of symbol_inittext_end
 *  (2). value of symbol_sys_end
 *  
 ******************************************************************************/
static void __init mem_init(void) 
{
    /* we need to turn on the A20 in some hardware. */
    u8 outport_cmd = kbdctrl_outport_r();
    if ((outport_cmd & 0x02) == 0)
    {
        kbdctrl_outport_w(outport_cmd | 0x02);
        printf("Just turn on the A20.\n");
    }

    a20enable_test();


    /* system ram space + extend ram space */
    ram_size = 1024 * 1024 + 
               ((struct raminfo *)raminfo_buf)->ax * 1024 + 
               ((struct raminfo *)raminfo_buf)->bx * 1024 * 64;

    /* some body has the M.K ram size? */
    /* ASSERT((ram_size & (1024 * 1024 - 1)) == 0); */
    printf("Detected ram size is :0x%#8x, ", ram_size);
    if ((ram_size & (1024 * 1024 - 1)) != 0)
    {
        ram_size &= (~(1024 * 1024 - 1));
    }
    if (ram_size > 64 * 1024 * 1024)
    {
        ram_size = 64 * 1024 * 1024;
    }
    printf("we used 0x%#8x\n", ram_size);
    

    printf("Total RAM space: %dM.\n", ram_size >> 20);
    printf("The section '.init.text' end position: %#8x.\n", GET_SYMBOLVALUE(symbol_inittext_end));
    printf("System end position: %#8x, Total size: %dK\n", GET_SYMBOLVALUE(symbol_sys_end), GET_SYMBOLVALUE(symbol_sys_end) / 1024);

    /* build the free page buddy system. */
    buddy_init();
}

module_init(mem_init, 0);

