#include "typedef.h"
#include "config.h"
#include "hd.h"
#include "command.h"
#include "debug.h"
#include "memory.h"
#include "module.h"
#include "crc32.h"
#include "partition.h"

unsigned    n_supportfs;             /* num of fs we suppored */

static u8 file_attrib[8][4] = {{"---\0"},         /* 0x0 */
                               {"--x\0"},         /* 0x1 */
                               {"-w-\0"},         /* 0x2 */
                               {"-wx\0"},         /* 0x3 */
                               {"r--\0"},         /* 0x4 */
                               {"r-x\0"},         /* 0x5 */
                               {"rw-\0"},         /* 0x6 */
                               {"rwx\0"}};        /* 0x7 */

static void cmd_ls_opfunc(char *argv[], int argc, void *param)
{
    if (cursel_partition == NULL)
    {
        return;
    }
    stat_dentry_array_t *stat_dentry = cursel_partition->fs->fs_stat(cursel_partition->fs, cursel_partition);
    if (stat_dentry == NULL)
        return;

    /* print all entrys */
    u32 idx;
    for (idx = 0; idx < (stat_dentry->n_dentry); idx++)
    {
        dentry_t *dentry = stat_dentry->dentry_array + idx;
        u32 attrib = dentry->inode.i_mode;
        if (S_ISDIR(attrib))
            printk("d");
        else
            printk("-");
        /* user attribute */
        printk("%s", file_attrib[(S_IRWXU & attrib) >> 6]);
        /* grp attribute */
        printk("%s", file_attrib[(S_IRWXG & attrib) >> 3]);
        /* oth attribute */
        printk("%s\t\t", file_attrib[(S_IRWXO & attrib) >> 0]);

        /* uid gid size */
        printk("%d\t%d%10d\t", dentry->inode.i_uid,
                                 dentry->inode.i_gid,
                                 dentry->inode.i_size);

        /* name */
        printk("%s\n", dentry->name);
    }

    page_free(stat_dentry);
}

struct command cmd_ls _SECTION_(.array.cmd) =
{
    .cmd_name   = "ls",
    .info       = "List all files under current directory.",
    .op_func    = cmd_ls_opfunc,
};

static void cmd_cd_opfunc(char *argv[], int argc, void *param)
{
    if (cursel_partition == NULL)
    {
        return;
    }
    
    if (cursel_partition->fs->fs_changecurdir(cursel_partition->fs, cursel_partition, argv[1]))
    {
        printk("No such directory.\n");
    }
}

struct command cmd_cd _SECTION_(.array.cmd) =
{
    .cmd_name   = "cd",
    .info       = "Change current directory.",
    .op_func    = cmd_cd_opfunc,
};

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

/*  */
int fs_readfile(struct partition_desc *part, char *name, void *buf)
{
	ASSERT((part != NULL) && (name != NULL));

	/* stat current path, we want to find the file */
    stat_dentry_array_t *stat_dentry = part->fs->fs_stat(part->fs, part);
    if (stat_dentry == NULL)
        return -1;

    u32 idx, dstinode = 0;
    for (idx = 0; idx < (stat_dentry->n_dentry); idx++)
    {
        dentry_t *dentry = stat_dentry->dentry_array + idx;
		if (strlen(dentry->name) != strlen(name))
			continue;
		if (memcmp(dentry->name, name, strlen(name)) == 0)
		{
			dstinode = dentry->inode_idx;
			break;
		}
    }
	page_free(stat_dentry);

	if (idx == stat_dentry->n_dentry)
	{
		return -1;
	}

	/* let's read the inode of file */
	inode_t inode;
	part->fs->fs_getinode(part->fs, part, &inode, dstinode);

	/* load the file */
	if (buf)
		part->fs->fs_readfile(part->fs, part, &inode, buf);

	return inode.i_size;
}

/* boot the kernel
 * argv[1]: dest kernel filename
 */

static void cmd_boot_opfunc(char *argv[], int argc, void *param)
{
	/* now we just support the file under current path */
    if (cursel_partition == NULL)
    {
        return;
    }

	/* load the file */
	void *bootmem = page_alloc(BUDDY_RANK_32M, MMAREA_NORMAL);
	/* let's just direct mmap 32M */
	// mmap((u32)bootmem, (u32)bootmem, 32 * 1024 * 1024, 0);

	int filesize = fs_readfile(cursel_partition, argv[1], bootmem);
	if (filesize < 0)
	{
		printk("Boot kernel file not found.\n");
		return;
	}
	printk("file %s loaded at 0x%#8x, total %d bytes.\n", argv[1], (u32)bootmem, filesize);
#if 0
	/* stat current path, we want to find the file */
    struct page *dentry_bufpage = cursel_partition->fs->fs_stat(cursel_partition->fs, cursel_partition);
    if (dentry_bufpage == NULL)
        return;

    stat_dentry_array_t *stat_dentry = (stat_dentry_array_t *)page2phyaddr(dentry_bufpage);
    u32 idx, dstinode = 0;
    for (idx = 0; idx < (stat_dentry->n_dentry); idx++)
    {
        dentry_t *dentry = stat_dentry->dentry_array + idx;
		if (strlen(dentry->name) != strlen(argv[1]))
			continue;
		if (memcmp(dentry->name, argv[1], strlen(argv[1])) == 0)
		{
			dstinode = dentry->inode_idx;
			break;
		}
    }
	page_free(dentry_bufpage);

	if (idx == stat_dentry->n_dentry)
	{
		printk("Boot kernel file not found.\n");
		return;
	}

	/* let's read the inode of file */
	inode_t inode;
	cursel_partition->fs->fs_getinode(cursel_partition->fs, cursel_partition, &inode, dstinode);
	/* load the file */
	void *bootmem = page2phyaddr(page_alloc(BUDDY_RANK_32M));
	/* let's just direct mmap 32M */
	mmap((u32)bootmem, (u32)bootmem, 32 * 1024 * 1024, 0);
	cursel_partition->fs->fs_readfile(cursel_partition->fs, cursel_partition, &inode, bootmem);
#endif

	linux_boothead_t *boothead = (void *)bootmem;

	/* copy the setup and kernel
	 * setup  ---> any address below 1M
	 * kernel ---> 1M
	 */

	printk("setup sector number: %d\n", boothead->setup_sects);

	memcpy((void *)0x70000, bootmem, (boothead->setup_sects + 1) * 512);

/*
	memcpy((void *)0x100000,
		   (void *)((u32)bootmem + (boothead->setup_sects + 1) * 512),
		   inode.i_size - (boothead->setup_sects + 1) * 512);
*/
	printk("filesize - (boothead->setup_sects + 1) * 512: %d\n"
		   "restore the IDTR ---> 0x%#2x 0x%#2x 0x%#2x 0x%#2x 0x%#2x 0x%#2x\n"
		   "myLoader exit...\n",
		   filesize - (boothead->setup_sects + 1) * 512,
		   g_bootp->idtr_back[0], g_bootp->idtr_back[1], g_bootp->idtr_back[2],
		   g_bootp->idtr_back[3], g_bootp->idtr_back[4], g_bootp->idtr_back[5]);

	boothead = (linux_boothead_t *)0x70000;
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

		:"=m"(g_bootp->idtr_back)
		:
		:"%ax"
	);
}

struct command cmd_boot _SECTION_(.array.cmd) =
{
    .cmd_name   = "boot",
    .info       = "boot kernel file.",
    .op_func    = cmd_boot_opfunc,
};

/* build the file system, try to load the main fs */
static void __init fs_init(void)
{
    struct file_system *fs;
    unsigned i;
    n_supportfs = (GET_SYMBOLVALUE(fsdesc_array_end) - (unsigned)fsdesc_array) / sizeof(struct file_system);
    DEBUG("n_supportfs: %d\n", n_supportfs);
    /* install all fs */
    for (i = 0; i < n_supportfs; i++)
    {
        fs = &(fsdesc_array[i]);
        fs->regflag = 0;
        if ((fs->fs_mount == NULL) ||
            (fs->fs_unmount == NULL) ||
            (fs->fs_register == NULL))
        {
            continue;
        }

        if (fs->fs_register(fs, NULL) == 0)
        {
            /* file system registed success. */
            fs->regflag = 1;
        }
    }
}

module_init(fs_init, 2);

