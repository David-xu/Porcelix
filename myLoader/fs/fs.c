#include "typedef.h"
#include "fs.h"
#include "hd.h"
#include "command.h"
#include "debug.h"

unsigned    n_supportfs;             /* num of fs we suppored */

void fs_init(void)
{
    struct file_system *fs;
    unsigned i;
    n_supportfs = (GET_SYMBOLVALUE(fsdesc_array_end) - (unsigned)fsdesc_array) / sizeof(struct file_system);
    DEBUG("n_supportfs: %d\n", n_supportfs);
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

static u8 file_attrib[8][4] = {{"---\0"},         /* 0x0 */
                               {"--x\0"},         /* 0x1 */
                               {"-w-\0"},         /* 0x2 */
                               {"-wx\0"},         /* 0x3 */
                               {"r--\0"},         /* 0x4 */
                               {"r-x\0"},         /* 0x5 */
                               {"rw-\0"},         /* 0x6 */
                               {"rwx\0"}};        /* 0x7 */

static void cmd_ls_opfunc(u8 *argv[], u8 argc, void *param)
{
    if (cursel_partition == NULL)
    {
        return;
    }
    struct page *dentry_bufpage = cursel_partition->fs->fs_stat(cursel_partition->fs, cursel_partition);
    if (dentry_bufpage == NULL)
        return;

    /* print all entrys */
    stat_dentry_array_t *stat_dentry = (stat_dentry_array_t *)get_phyaddr(dentry_bufpage);
    u32 idx;
    for (idx = 0; idx < (stat_dentry->n_dentry); idx++)
    {
        dentry_t *dentry = stat_dentry->dentry_array + idx;
        u32 attrib = dentry->inode.i_mode;
        if (S_ISDIR(attrib))
            printf("d");
        else
            printf("-");
        /* user attribute */
        printf("%s", file_attrib[(S_IRWXU & attrib) >> 6]);
        /* grp attribute */
        printf("%s", file_attrib[(S_IRWXG & attrib) >> 3]);
        /* oth attribute */
        printf("%s\t\t", file_attrib[(S_IRWXO & attrib) >> 0]);

        /* uid gid size */
        printf("%d\t%d%10d\t", dentry->inode.i_uid,
                                 dentry->inode.i_gid,
                                 dentry->inode.i_size);

        /* name */
        printf("%s\n", dentry->name);
    }

    page_free(dentry_bufpage);
}

static void cmd_cd_opfunc(u8 *argv[], u8 argc, void *param)
{
    if (cursel_partition == NULL)
    {
        return;
    }
    
    if (cursel_partition->fs->fs_changecurdir(cursel_partition->fs, cursel_partition, argv[1]))
    {
        printf("No such directory.\n");
    }
}

struct command cmd_ls _SECTION_(.array.cmd) =
{
    .cmd_name   = "ls",
    .info       = "List all files under current directory.",
    .op_func    = cmd_ls_opfunc,
};

struct command cmd_cd _SECTION_(.array.cmd) =
{
    .cmd_name   = "cd",
    .info       = "Change current directory.",
    .op_func    = cmd_cd_opfunc,
};

