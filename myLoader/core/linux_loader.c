#include "public.h"
#include "assert.h"
#include "io.h"
#include "ml_string.h"
#include "boot.h"
#include "crc32.h"
#include "memory.h"

/*
	~                        ~
		|  Protected-mode kernel |
100000  +------------------------+
	|  I/O memory hole	 |
0A0000	+------------------------+
	|  Reserved for BIOS	 |	Leave as much as possible unused
	~                        ~
	|  Command line		 |	(Can also be below the X+10000 mark)
X+10000	+------------------------+
	|  Stack/heap		 |	For use by the kernel real-mode code.
X+08000	+------------------------+
	|  Kernel setup		 |	The kernel real-mode code.
	|  Kernel boot sector	 |	The kernel legacy boot sector.
X       +------------------------+
	|  Boot loader		 |	<- Boot sector entry point 0000:7C00
001000	+------------------------+
	|  Reserved for MBR/BIOS |
000800	+------------------------+
	|  Typically used by MBR |
000600	+------------------------+
	|  BIOS use only	 |
000000	+------------------------+
*/

#define		LOADED_HIGH				0x01
#define		QUIET_FLAG				0x20
#define		KEEP_SEGMENTS			0x40
#define		CAN_USE_HEAP			0x80
static void prepare_kcmdline(linux_boothead_t *bootp)
{
	bootp->cmd_line_ptr = 0x7e000;
	memset((void *)(bootp->cmd_line_ptr), 0, 1024);
	sprintf((char *)bootp->cmd_line_ptr, // "earlyprintk=serial,ttyS0,115200 "
		                                 // "console=ttyS0,115200 "
		                                 // "CONSOLE=/dev/tty1"
		                                 "apic=verbose "
		                                 "debug "
		                                 // "hd=130,16,63 "
		                                 "root=/dev/hda1 rw "
		                                 // "keep_bootcon "
		                                 "nosmep "				/* if not set, kernel boot from VMware will failed, I don't know why. */
		                                 "init=/sbin/init"
		                                 );
}

static void prepare_ramdisk(linux_boothead_t *bootp)
{
	bootp->ramdisk_image = 0;
	bootp->ramdisk_size = 0;
}

void linux_start(void *bootmem, u32 size)
{
	linux_boothead_t *boothead = bootmem;

	/* copy the setup and kernel
	 * setup  ---> any address below 1M
	 * kernel ---> 1M
	 */

	// boothead = page_alloc(get_pagerank(size), MMAREA_NORMAL);
	boothead = (linux_boothead_t *)V2P(0x70000);
	memcpy(boothead, bootmem,
		(((linux_boothead_t *)bootmem)->setup_sects + 1) * 512);

/*
	memcpy((void *)0x100000,
		   (void *)((u32)bootmem + (boothead->setup_sects + 1) * 512),
		   inode.i_size - (boothead->setup_sects + 1) * 512);
*/
	printk("restore the IDTR ---> 0x%#2x 0x%#2x 0x%#2x 0x%#2x 0x%#2x 0x%#2x\n"
		   "myLoader exit...\n",
		   g_bootp->idtr_back[0], g_bootp->idtr_back[1], g_bootp->idtr_back[2],
		   g_bootp->idtr_back[3], g_bootp->idtr_back[4], g_bootp->idtr_back[5]);


	/* kernel cmd line */
	prepare_kcmdline(boothead);

	prepare_ramdisk(boothead);

	/* kernel core load addr */
	boothead->code32_start = (u32)bootmem + (boothead->setup_sects + 1) * 512;

	boothead->vid_mode = NORMAL_VGA;	// NORMAL_VGA EXTENDED_VGA ASK_VGA
	boothead->type_of_loader = 0xFF;
	boothead->loadflags |= LOADED_HIGH | CAN_USE_HEAP;
	boothead->heap_end_ptr = boothead->cmd_line_ptr - 0x200;

	/* now we prepare to switch to realmode, make sure the BIOS's RAM(0~0x1000 4K) we have not changed yet. */
	ASSERT(g_bootp->head4k_crc == crc32((void *)0, 0x1000));

	/* switch to realmode */
	asm volatile (
		/* 1. disable interrupt */
		"cli							\n\t"

		/* 2. disable paging */
		"movl	%%cr0, %%eax			\n\t"
		"andl   $0x7FFFFFFF, %%eax		\n\t"
		"movl	%%eax, %%cr0			\n\t"

		//"movl	$0, %%cr3				\n\t"
		/* 3. load the IDTR of the BIOS which backup in bootparam.idtr_back */
		"lidt	%0						\n\t"


		/**/
		"jmp	swith2realmode			\n\t"

		:
		:"m"(g_bootp->idtr_back)
		:"%ax"
	);
}

