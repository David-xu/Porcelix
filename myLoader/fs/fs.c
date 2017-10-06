#include "typedef.h"
#include "config.h"
#include "hd.h"
#include "command.h"
#include "debug.h"
#include "memory.h"
#include "module.h"
#include "crc32.h"
#include "partition.h"
#include "loader.h"

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
 * argv[1]: -ram | -disk
 * argv[2]: dest kernel filename
 */

static void cmd_boot_opfunc(char *argv[], int argc, void *param)
{
	void *bootmem;
	int filesize;

	printk("Boot linux kernel...\n");

	if (memcmp("-disk", argv[1], sizeof("-disk")) == 0)
	{
		/* now we just support the file under current path */
	    if (cursel_partition == NULL)
	    {
	        return;
	    }

		/* load the file */
		bootmem = page_alloc(BUDDY_RANK_32M, MMAREA_NORMAL);
		/* let's just direct mmap 32M */
		// mmap((u32)bootmem, (u32)bootmem, 32 * 1024 * 1024, 0);

		filesize = fs_readfile(cursel_partition, argv[2], bootmem);
		if (filesize < 0)
		{
			printk("Boot kernel file not found.\n");
			return;
		}
		printk("file %s loaded at 0x%#8x, total %d bytes.\n", argv[2], (u32)bootmem, filesize);

		linux_start(bootmem, filesize);
	}
	else if (memcmp("-ram", argv[1], sizeof("-ram")) == 0)
	{
		bootmem = (void *)str2num(argv[2]);
		filesize = str2num(argv[3]);

		printk("ram addr 0x%#8x, total %d bytes.\n", bootmem, filesize);
		linux_start(bootmem, filesize);
	}
	else
	{

	}


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

