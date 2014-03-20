#ifndef _EXT2_H_
#define _EXT2_H_

#include "typedef.h"
#include "memory.h"
#include "fs.h"

#define EXT2_NAME_LEN           (255)

#define EXT2_ROOT_INODEIDX      (2)

#define EXT2_MAGICNUM           (0xEF53)

#define EXT2_MAXDIRSIZE         (1024)


/*
 * Structure of a blocks group descriptor
 * actrually I copied it from linux kernel
 */
struct ext2_group_desc
{
    u32     bg_block_bitmap;		/* Blocks bitmap block */
    u32     bg_inode_bitmap;		/* Inodes bitmap block */
    u32     bg_inode_table;		/* Inodes table block */
    u16     bg_free_blocks_count;	/* Free blocks count */
    u16     bg_free_inodes_count;	/* Free inodes count */
    u16     bg_used_dirs_count;	/* Directories count */
    u16     bg_pad;
    u32     bg_reserved[3];
};


/*
 * Constants relative to the data blocks
 * actrually I copied it from linux kernel
 */
#define	EXT2_NDIR_BLOCKS        12
#define	EXT2_IND_BLOCK          EXT2_NDIR_BLOCKS            /* rand 1 */
#define	EXT2_DIND_BLOCK         (EXT2_IND_BLOCK + 1)        /* rand 2 */
#define	EXT2_TIND_BLOCK         (EXT2_DIND_BLOCK + 1)       /* rand 3 */
#define	EXT2_N_BLOCKS           (EXT2_TIND_BLOCK + 1)


/*
 * Structure of an inode on the disk
 * actrually I copied it from linux kernel
 */
struct ext2_inode {
	u16	i_mode;		/* File mode */
	u16	i_uid;		/* Low 16 bits of Owner Uid */
	u32	i_size;		/* Size in bytes */
	u32	i_atime;	/* Access time */
	u32	i_ctime;	/* Creation time */
	u32	i_mtime;	/* Modification time */
	u32	i_dtime;	/* Deletion Time */
	u16	i_gid;		/* Low 16 bits of Group Id */
	u16	i_links_count;	/* Links count */
	u32	i_blocks;	/* Blocks count */
	u32	i_flags;	/* File flags */
    union
    {
        struct
        {
            u32 l_i_reserved1;
        } linux1;
        struct
        {
            u32 h_i_translator;
        } hurd1;
        struct
        {
            u32 m_i_reserved1;
        } masix1;
    } osd1;				/* OS dependent 1 */
    u32     i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
    u32     i_generation;	/* File version (for NFS) */
    u32     i_file_acl;	/* File ACL */
    u32     i_dir_acl;	/* Directory ACL */
    u32     i_faddr;	/* Fragment address */
	union
    {
		struct
        {
			u8      l_i_frag;	/* Fragment number */
			u8      l_i_fsize;	/* Fragment size */
			u16     i_pad1;
			u16     l_i_uid_high;	/* these 2 fields    */
			u16     l_i_gid_high;	/* were reserved2[0] */
		    u32     l_i_reserved2;
		} linux2;
		struct
        {
			u8      h_i_frag;	/* Fragment number */
			u8      h_i_fsize;	/* Fragment size */
			u16     h_i_mode_high;
			u16     h_i_uid_high;
			u16     h_i_gid_high;
			u32     h_i_author;
		} hurd2;
		struct
        {
			u8      m_i_frag;	/* Fragment number */
			u8      m_i_fsize;	/* Fragment size */
			u16     m_pad1;
			u32     m_i_reserved2[2];
		} masix2;
	} osd2;				/* OS dependent 2 */
};

/*
 * The new version of the directory entry.  Since EXT2 structures are
 * stored in intel byte order, and the name_len field could never be
 * bigger than 255 chars, it's safe to reclaim the extra byte for the
 * file_type field.
 */
struct ext2_dir_entry_2 {
    u32     inode;			/* Inode number */
    u16     rec_len;		/* Directory entry length */
    u8      name_len;		/* Name length */
    u8      file_type;
	char	name[];			/* File name, up to EXT2_NAME_LEN */
};


/*
 * Structure of the super block
 * actrually I copied it from linux kernel
 */
struct ext2_superblock{
    u32     s_inodes_count;		/* Inodes count */
    u32     s_blocks_count;		/* Blocks count */
    u32     s_r_blocks_count;	/* Reserved blocks count */
    u32     s_free_blocks_count;	/* Free blocks count */
    u32     s_free_inodes_count;	/* Free inodes count */
    u32     s_first_data_block;	/* First Data Block */
    u32     s_log_block_size;	/* Block size */
    u32     s_log_frag_size;	/* Fragment size */
    u32     s_blocks_per_group;	/* # Blocks per group */
    u32     s_frags_per_group;	/* # Fragments per group */
    u32     s_inodes_per_group;	/* # Inodes per group */
    u32     s_mtime;		/* Mount time */
    u32     s_wtime;		/* Write time */
    u16     s_mnt_count;		/* Mount count */
    u16     s_max_mnt_count;	/* Maximal mount count */
    u16     s_magic;		/* Magic signature */
    u16     s_state;		/* File system state */
    u16     s_errors;		/* Behaviour when detecting errors */
    u16     s_minor_rev_level; 	/* minor revision level */
    u32     s_lastcheck;		/* time of last check */
    u32     s_checkinterval;	/* max. time between checks */
    u32     s_creator_os;		/* OS */
    u32     s_rev_level;		/* Revision level */
    u16     s_def_resuid;		/* Default uid for reserved blocks */
    u16     s_def_resgid;		/* Default gid for reserved blocks */
	/*
	 * These fields are for EXT2_DYNAMIC_REV superblocks only.
	 *
	 * Note: the difference between the compatible feature set and
	 * the incompatible feature set is that if there is a bit set
	 * in the incompatible feature set that the kernel doesn't
	 * know about, it should refuse to mount the filesystem.
	 * 
	 * e2fsck's requirements are more strict; if it doesn't know
	 * about a feature in either the compatible or incompatible
	 * feature set, it must abort and not try to meddle with
	 * things it doesn't understand...
	 */
    u32     s_first_ino; 		/* First non-reserved inode */
    u16     s_inode_size; 		/* size of inode structure */
    u16     s_block_group_nr; 	/* block group # of this superblock */
    u32     s_feature_compat; 	/* compatible feature set */
    u32     s_feature_incompat; 	/* incompatible feature set */
    u32     s_feature_ro_compat; 	/* readonly-compatible feature set */
    u8      s_uuid[16];		/* 128-bit uuid for volume */
	char	s_volume_name[16]; 	/* volume name */
	char	s_last_mounted[64]; 	/* directory where last mounted */
    u32     s_algorithm_usage_bitmap; /* For compression */
	/*
	 * Performance hints.  Directory preallocation should only
	 * happen if the EXT2_COMPAT_PREALLOC flag is on.
	 */
    u8      s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
    u8      s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
    u16     s_padding1;
	/*
	 * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
	 */
    u8      s_journal_uuid[16];	/* uuid of journal superblock */
    u32     s_journal_inum;		/* inode number of journal file */
    u32     s_journal_dev;		/* device number of journal file */
    u32     s_last_orphan;		/* start of list of inodes to delete */
    u32     s_hash_seed[4];		/* HTREE hash seed */
    u8      s_def_hash_version;	/* Default hash version to use */
    u8      s_reserved_char_pad;
    u16     s_reserved_word_pad;
    u32     s_default_mount_opts;
    u32     s_first_meta_bg; 	/* First metablock block group */
    u32     s_reserved[190];	/* Padding to the end of the block */
};


/* this is the fsparam, the pointer of this struct will be
 * stored in the 
 */
struct ext2_info{
    struct page *fsparam;           /* this page used to store some fs param like
                                       superblock grp_desc block_bitmap inode_bitmap */
    struct ext2_superblock *sb;     /* 1K */
    struct ext2_group_desc *grp_desc;   /* 1K */
    void *fsblock_buff01;           /* 1K */
    void *fsblock_buff02;           /* 1K */

    u32 blk_size;                   /* filesystem block size */

    u8 inode_per_block;

    u32         curdir_idx;         /* the index of the dentry_buf */
    dentry_t    dentry_buf[0];
};


#endif

