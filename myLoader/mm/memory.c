#include "typedef.h"
#include "memory.h"
#include "debug.h"
#include "assert.h"
#include "command.h"
#include "debug.h"
#include "string.h"

static void cmd_ramdisp_opfunc(u8 *argv[], u8 argc, void *param)
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
u8 raminfo_buf[RAMINFO_MAXLEN];

u32 ram_size;           /* we call the bios to get this param. */


/******************************************************************************
 *               (1)                (2)
 *   (.init.text) |     system       |   hole   |   page_list   |
 *  (1). value of symbol_inittext_end
 *  (2). value of symbol_sys_end
 *  
 ******************************************************************************/
void mem_init(void)
{
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

    buddy_init();
}

