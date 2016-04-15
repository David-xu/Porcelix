#include "typedef.h"
#include "assert.h"
#include "section.h"
#include "fs.h"
#include "ext2.h"
#include "io.h"
#include "hd.h"
#include "memory.h"
#include "public.h"
#include "ml_string.h"
#include "device.h"
#include "block.h"
#include "debug.h"
#include "partition.h"

#if CONFIG_FSDEBUG_DUMP
static void ext2_dump_inode(struct ext2_inode *ext2_inode)
{
    printk("Ext2 fs inode:\n"
           "inode mode          : 0x%#4x\n"
           "uid                 : %d\n"
           "gid                 : %d\n"
           "i_size              : %d\n"
           "Block idx: 0x%#8x 0x%#8x 0x%#8x 0x%#8x\n",
           ext2_inode->i_mode,
           ext2_inode->i_uid,
           ext2_inode->i_gid,
           ext2_inode->i_size,
           ext2_inode->i_block[0], ext2_inode->i_block[1],
           ext2_inode->i_block[2], ext2_inode->i_block[3]);
}

static void ext2_dump_sb(struct ext2_superblock *ext2_sb)
{
    printk("Ext2 fs superblock info:\n"
           "Total blocks        : 0x%#8x\n"
           "Blocks per blockgrp : 0x%#8x\n"
           "First_data_block    : 0x%#8x\n"
           "Block size          : %d\n"
           "Inode per group     : %d\n",
           ext2_sb->s_blocks_count,
           ext2_sb->s_blocks_per_group,
           ext2_sb->s_first_data_block,
           0x1 << (10 + ext2_sb->s_log_block_size),
           ext2_sb->s_inodes_per_group);
}

static void ext2_dump_grpdesc(struct ext2_group_desc *ext2_grpdesc, u32 n_grp)
{
    u32 count;
    for (count = 0; count < n_grp; count++)
    {
        printk("grp %d:\n"
               "bg_block_bitmap   : 0x%#8x\n"
               "bg_block_bitmap   : 0x%#8x\n"
               "bg_inode_table    : 0x%#8x\n"
               "free_blocks_count : %d\n"
               "free_inodes_count : %d\n",
               count,
               ext2_grpdesc[count].bg_block_bitmap,
               ext2_grpdesc[count].bg_inode_bitmap,
               ext2_grpdesc[count].bg_inode_table,
               ext2_grpdesc[count].bg_free_blocks_count,
               ext2_grpdesc[count].bg_free_inodes_count);
        // while (-1 == kbd_get_char());
    }
}

static void ext2_dump_info(struct ext2_info *info)
{
    printk("Ext2fs info:\n"
           "sb_addr           : 0x%#8x\n"
           "grp_desc_addr     : 0x%#8x\n"
           "fsblock_buff_addr1: 0x%#8x\n"
           "fsblock_buff_addr2: 0x%#8x\n"
           "inode_per_block   : %d\n",
           (u32)(info->sb),
           (u32)(info->grp_desc),
           (u32)(info->fsblock_buff01),
           (u32)(info->fsblock_buff02),
           info->inode_per_block);
}

#else

#define ext2_dump_inode(param...)
#define ext2_dump_sb(param...)
#define ext2_dump_grpdesc(param...)
#define ext2_dump_info(param...)

#endif

static int ext2_load_sb(struct file_system *fs, void *param)
{
    struct partition_desc *part_desc = (struct partition_desc *)param;
    device_t *dev = part_desc->dev;

    /* 1K(superblock)     +
       1K(grp_desc)       +
       1K(fsblock_buff01) +
       1K(fsblock_buff02) +
       ext2_info */
    void *fsparam = page_alloc(BUDDY_RANK_8K, MMAREA_NORMAL);
    if (fsparam == NULL)
    {
        return -1;
    }
    /* locate the ext2_info position */
    struct ext2_info *info = (struct ext2_info *)(fsparam + PAGE_SIZE);
    /* ext2_info parameter set */
    info->fsparam = fsparam;
    info->sb = (struct ext2_superblock *)fsparam;

    /* get the superblock, we have to locate the position */
    dev->driver->read(dev, 1, info->sb, 1);
    /* check the magic num, make sure this is the ext2 filesystem. */
    if (info->sb->s_magic != EXT2_MAGICNUM)
    {
        printk("ext2 super_block broken, invalid magic.\n");
        page_free(fsparam);     /* free fsparam page */
        return -1;
    }
    /* check the inode size */
    if (info->sb->s_inode_size != sizeof(struct ext2_inode))
    {
        printk("ext2 super_block broken, invalid inodesize.\n");
        page_free(fsparam);     /* free fsparam page */
        return -1;
    }    
    ext2_dump_sb(info->sb);

    /* caculate the grp_desc position and read the grp_desc */
    u32 grpdesc_logicbkidx = 2;
    info->grp_desc = (struct ext2_group_desc *)((u32)(info->sb) + LOGIC_BLOCK_SIZE);
    if (info->sb->s_log_block_size == 2)
    {
        grpdesc_logicbkidx = 4;     /* if the fs block size is 4 * LOGIC_BLOCK_SIZE
                                     * the grp_desc is locate at the place of logic block 4
                                     */
    }
    /* read grp_desc into the grp_desc */
    dev->driver->read(dev, grpdesc_logicbkidx, info->grp_desc, 1);
    
#if 0
    ext2_dump_grpdesc(info->grp_desc,
                      (info->sb->s_blocks_count + info->sb->s_blocks_per_group - 1)
                      / info->sb->s_blocks_per_group);
#endif

    
    info->fsblock_buff01 = (void *)((u32)(info->grp_desc) + LOGIC_BLOCK_SIZE);
    info->fsblock_buff02 = (void *)((u32)(info->grp_desc) + 2 * LOGIC_BLOCK_SIZE);

    info->blk_size = (0x1 << (10 + info->sb->s_log_block_size));

    /* set the n_inode per block, used it to locate one special inode */
    info->inode_per_block = info->blk_size / info->sb->s_inode_size;

    /**/
    part_desc->param = info;

    ext2_dump_info(info);
    
    return 0;
}

/* ext2_load_block(), we have all the ext2 fs param, so we can locate the sect
 * with the block by all the param
 */
static int ext2_load_block(struct file_system *fs, void *param, u32 blockidx, void *buff)
{
    DEBUG("ext2_load_block blkinx:0x%#8x\n", blockidx);
    struct partition_desc *part_desc = (struct partition_desc *)param;
    struct ext2_info *info = (struct ext2_info *)(part_desc->param);
    device_t *dev = part_desc->dev;

    return dev->driver->read(dev, blockidx * info->blk_size / LOGIC_BLOCK_SIZE,
                             buff,
                             info->blk_size / LOGIC_BLOCK_SIZE);
}

static int ext2_get_inode(struct file_system *fs, void *param, inode_t *inode, u32 inode_idx)
{
    struct partition_desc *part_desc = (struct partition_desc *)param;
    struct ext2_info *info = (struct ext2_info *)(part_desc->param);

    struct ext2_group_desc *curgrp;
    u32 inode_idxingrp, inode_bkidx, inode_offsetinbk;

    /* locate the inode */
    curgrp = info->grp_desc + ((inode_idx - 1) / (info->sb->s_inodes_per_group));
    inode_idxingrp = (inode_idx - 1) % (info->sb->s_inodes_per_group);
    inode_bkidx = curgrp->bg_inode_table +
                  inode_idxingrp / info->inode_per_block;
    inode_offsetinbk = inode_idxingrp % info->inode_per_block;

    /* load the block with the inode struct */
    ext2_load_block(fs, param, inode_bkidx, info->fsblock_buff02);    
    struct ext2_inode *ext2_inode = (struct ext2_inode *)(info->fsblock_buff02) + inode_offsetinbk;

    /* cp the ext2inode data into inode */
    inode->i_mode = ext2_inode->i_mode;
    inode->i_uid = ext2_inode->i_uid;
    inode->i_size = ext2_inode->i_size;
    inode->i_gid = ext2_inode->i_gid;    
    inode->i_blocks = ext2_inode->i_blocks;
    inode->i_atime = ext2_inode->i_atime;
    inode->i_ctime = ext2_inode->i_ctime;
    inode->i_mtime = ext2_inode->i_mtime;

    memcpy(inode->i_block, ext2_inode->i_block, sizeof(ext2_inode->i_block));
    
    return 0;
}

static int ext2_register(struct file_system *fs, void *param)
{
    ASSERT(sizeof(struct ext2_superblock) == (0x1 << 10));
    DEBUG("\"%s\" has been registed.\n", fs->name);
    return 0;
}

/*
 * param: device_t
 */
static int ext2_mount(struct file_system *fs, void *param)
{
    if (ext2_load_sb(fs, param) != 0)
    {
        DEBUG("ext2 super_block load failed.\n");
        return -1;
    }

    struct partition_desc *part_desc = (struct partition_desc *)param;
    struct ext2_info *info = (struct ext2_info *)(part_desc->param);

    /* load the root dir */
    info->curdir_idx = 0;
    dentry_t *dentry = info->dentry_buf;
    memset(dentry->name, 0, sizeof(dentry->name));
    dentry->inode_idx = EXT2_ROOT_INODEIDX;
    ext2_get_inode(fs, param, &(dentry->inode), EXT2_ROOT_INODEIDX);

    return 0;
}

static int ext2_unmount(struct file_system *fs, void *param)
{
    struct partition_desc *part_desc = (struct partition_desc *)param;
    struct ext2_info *fsinfo = (struct ext2_info *)(part_desc->param);

    /* did not yet mount filesystem */
    if (part_desc->fs == NULL)
    {
        return 0;
    }

    if (fsinfo->fsparam)
    {
        page_free(fsinfo->fsparam);
    }
    
    return 0;
}

static char *ext2_getcurdir(struct file_system *fs, void *param)
{
    struct partition_desc *part_desc = (struct partition_desc *)param;
    struct ext2_info *fsinfo = (struct ext2_info *)(part_desc->param);
    static char ext2_dirbuff[EXT2_MAXDIRSIZE];
    char *buf = ext2_dirbuff;
    *buf = '/';

    if (fsinfo->curdir_idx == 0)
    {
        *buf = '/';
        *(buf + 1) = '\0';
        return ext2_dirbuff;
    }
    u32 idx;
    for (idx = 1; idx <= fsinfo->curdir_idx; idx++)
    {
        sprintf(buf,
                "/%s",
                fsinfo->dentry_buf[idx].name);
        buf += 1 + strlen(fsinfo->dentry_buf[idx].name);
    }
    *buf = '\0';

    return ext2_dirbuff;
}

static stat_dentry_array_t* ext2_stat(struct file_system *fs, void *param)
{
    struct partition_desc *part_desc = (struct partition_desc *)param;
    struct ext2_info *fsinfo = (struct ext2_info *)(part_desc->param);
    
    /* get the all the entrys in the curdir */
    dentry_t *curdir = &(fsinfo->dentry_buf[fsinfo->curdir_idx]);
    /* this is one dir inode */
    ASSERT(S_ISDIR(curdir->inode.i_mode));

    /* stat the entry number */
    struct ext2_dir_entry_2 *entryp = fsinfo->fsblock_buff01;
    void *buff = entryp;
    u32 count, n_dentry = 0;
    for (count = 0; count < (curdir->inode.i_size / fsinfo->blk_size); count++)
    {
        ext2_load_block(fs, param, curdir->inode.i_block[count], buff);
        buff += fsinfo->blk_size;
    }
    count = curdir->inode.i_size;
    while (count)
    {
        count -= entryp->rec_len;
        entryp = (struct ext2_dir_entry_2 *)((u32)entryp + entryp->rec_len);
        n_dentry++;
    }

    /* get the dentry_array page rank */
    count = get_pagerank(n_dentry * sizeof(dentry_t));
    DEBUG("rank: %d\n", count);
    
    stat_dentry_array_t *stat_dentry = (stat_dentry_array_t *)page_alloc(count, MMAREA_NORMAL);
    stat_dentry->n_dentry = n_dentry;
    dentry_t *buffentry = stat_dentry->dentry_array;
    entryp = fsinfo->fsblock_buff01;      /* ext2 dentry buff */

    for (count = 0; count < n_dentry; count++)
    {
        memset(buffentry, 0, sizeof(dentry_t));

        /* copy the entry name */
        memcpy(buffentry->name, entryp->name, entryp->name_len);
        DEBUG("%s\n", buffentry->name);
        buffentry->inode_idx = entryp->inode;
        /* get the inode */
        ext2_get_inode(fs, param, &(buffentry->inode), buffentry->inode_idx);

        entryp = (struct ext2_dir_entry_2 *)((u32)entryp + entryp->rec_len);
        buffentry++;
    }

    return stat_dentry;
}

static int ext2_changecurdir(struct file_system *fs, void *param, char *dirname)
{
    struct partition_desc *part_desc = (struct partition_desc *)param;
    struct ext2_info *fsinfo = (struct ext2_info *)(part_desc->param);
    char *subdirname[16], n_subdir;
	int ret = 0;
    stat_dentry_array_t *stat_dentry = ext2_stat(fs, param);
    if (stat_dentry == NULL)
    {
        return -1;
    }

    /*  */
    if (dirname[0] == '/')
    {
        fsinfo->curdir_idx = 0;
    }

    n_subdir = parse_str_by_inteflag(dirname, subdirname, 2, "/\n");
    u32 count;
    for (count = 0; count < n_subdir; count++)
    {
        if (memcmp(subdirname[count], "..", 2) == 0)
        {
            if (fsinfo->curdir_idx)
                (fsinfo->curdir_idx)--;
            continue;
        }
        else if (memcmp(subdirname[count], ".", 1) == 0)
        {
            continue;
        }
        
        u32 entry_idx;
        dentry_t *listentry;
        for (entry_idx = 0; entry_idx < stat_dentry->n_dentry; entry_idx++)
        {
            listentry = stat_dentry->dentry_array + entry_idx;
            if (!S_ISDIR(listentry->inode.i_mode))
                continue;
            /* get the dst dentry indicated by subdirname[count] */
            if (memcmp(subdirname[count], listentry->name, strlen(listentry->name)) == 0)
            {
                DEBUG("entry name:%s\n", listentry->name);

                (fsinfo->curdir_idx)++;
                memcpy(fsinfo->dentry_buf + fsinfo->curdir_idx,
                       listentry,
                       sizeof(dentry_t));
                
                break;
            }
        }
        
        if (entry_idx == stat_dentry->n_dentry)
        {
            ret = -1;
			goto ext2_changecurdir_ret;
        }
    }

ext2_changecurdir_ret:
	page_free(stat_dentry);
    return ret;
}

/* return the number of fsblock we have readed */
static u32 ext2_readfile_ind(struct file_system *fs, void *param, u32 blkidx, u32 count, void *mem)
{
    struct partition_desc *part_desc = (struct partition_desc *)param;
    struct ext2_info *info = (struct ext2_info *)(part_desc->param);

	/* we need one page to store the indirect index */
	u32 ret, *blkidxarray = (u32 *)page_alloc(BUDDY_RANK_4K, MMAREA_NORMAL);

	ext2_load_block(fs, param, blkidx, blkidxarray);
	
	if (count > (info->blk_size / sizeof(u32)))
		ret = info->blk_size / sizeof(u32);
	else
		ret = count;

	/* now count is no need */
	for (count = 0; count < ret; count++)
	{
		ext2_load_block(fs, param, blkidxarray[count], (void *)((u32)mem + count * info->blk_size));
	}

	page_free(blkidxarray);

	return ret;
}

static u32 ext2_readfile_dind(struct file_system *fs, void *param, u32 blkidx, u32 count, void *mem)
{
    struct partition_desc *part_desc = (struct partition_desc *)param;
    struct ext2_info *info = (struct ext2_info *)(part_desc->param);

	/* we need one page to store the indirect index */
	u32 ret, idx, *blkidxarray = (u32 *)page_alloc(BUDDY_RANK_4K, MMAREA_NORMAL);
	ext2_load_block(fs, param, blkidx, blkidxarray);

	if (count > ((info->blk_size / sizeof(u32)) * (info->blk_size / sizeof(u32))))
		ret = (info->blk_size / sizeof(u32)) * (info->blk_size / sizeof(u32));
	else
		ret = count;

	count = ret;

	idx = 0;
	while (count)
	{
		count -= ext2_readfile_ind(fs, param, blkidxarray[idx], count, mem);

		mem = (void *)((u32)mem + (info->blk_size / sizeof(u32)) * info->blk_size);
		idx++;
	}
	ASSERT(idx <= (info->blk_size / sizeof(u32)));
	
	page_free(blkidxarray);

	return ret;
}

static int ext2_readfile(struct file_system *fs, void *param, inode_t *inode, void *mem)
{
    struct partition_desc *part_desc = (struct partition_desc *)param;
    struct ext2_info *info = (struct ext2_info *)(part_desc->param);

	u32 i, n_block, tmp = 0, addr = (u32)mem;

	/* let's calc the number of fsblock this file occupy */
	n_block = (inode->i_size + info->blk_size - 1) / info->blk_size;
	
	for (i = 0; i < EXT2_N_BLOCKS; i++)
	{
		/* direct index */
		if (i < EXT2_IND_BLOCK)
		{
			ext2_load_block(fs, param, inode->i_block[i], (void *)addr);
			tmp = 1;
		}
		/* index block */
		else if (i == EXT2_IND_BLOCK)
		{
			tmp = ext2_readfile_ind(fs, param, inode->i_block[i], n_block, (void *)addr);
		}
		/* double index */
		else if (i == EXT2_DIND_BLOCK)
		{
			tmp = ext2_readfile_dind(fs, param, inode->i_block[i], n_block, (void *)addr);
		}
		/* tri index */
		else
		{

		}

		addr += tmp * info->blk_size;
		n_block -= tmp;

		if (n_block == 0)
			break;
	}

	ASSERT(i != EXT2_N_BLOCKS);

	return 0;
}

struct file_system ext2_fs _SECTION_(.array.fs) = 
{
    .name               = "ext2",
    .fs_register        = ext2_register,
    .fs_mount           = ext2_mount,
    .fs_unmount         = ext2_unmount,

    .fs_getcurdir       = ext2_getcurdir,
    .fs_getinode        = ext2_get_inode,
    .fs_changecurdir    = ext2_changecurdir,
    .fs_stat            = ext2_stat,

	.fs_readfile		= ext2_readfile,
};
