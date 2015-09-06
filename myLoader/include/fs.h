#ifndef _FS_H_
#define _FS_H_

#include "typedef.h"
#include "section.h"
#include "list.h"

/* file stat, copied from linux kernel */
#define S_IFMT  00170000
#define S_IFSOCK 0140000
#define S_IFLNK	 0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000

#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#define FS_FILENAME_LEN             (64)
#define FS_INODE_BKMAPSIZE          (16)

/*  */
struct _inode;
typedef struct _file {
	struct _inode *inode;
} file_t;

typedef struct _inode {
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
    u32 i_block[FS_INODE_BKMAPSIZE];/* Pointers to blocks */
} inode_t;

typedef struct _dentry{
    char name[FS_FILENAME_LEN];
    u32 inode_idx;              /* this is the dir inode index */
    inode_t inode;              /* this is the dir inode buffer */
}dentry_t;

typedef struct _stat_dentry_array {
    u32 n_dentry;
    dentry_t dentry_array[0];
} stat_dentry_array_t;

/* To make it simple, we don't used the 'block layer'.
 */
struct file_system{
    u32 regflag         : 1;        /* this flag indicate the fs can be used. */
    u32 rsvflag         : 31;
    char *name;

    int (*fs_register)(struct file_system *fs, void *param);
    int (*fs_mount)(struct file_system *fs, void *param);
    int (*fs_unmount)(struct file_system *fs, void *param);

    int (*fs_getinode)(struct file_system *fs, void *param, inode_t *inode, u32 inode_idx);
    char* (*fs_getcurdir)(struct file_system *fs, void *param);
    int (*fs_changecurdir)(struct file_system *fs, void *param, char *dirname);
    stat_dentry_array_t* (*fs_stat)(struct file_system *fs, void *param);
	int (*fs_readfile)(struct file_system *fs, void *param, inode_t *inode, void *mem);

	/*  */
    u32 pad[6];
};

struct partition_desc;
int fs_readfile(struct partition_desc *part, char *name, void *buf);

// void fs_init(void) _SECTION_(.init.text);

/*  */
extern unsigned    n_supportfs;             /* num of fs we suppored */
extern struct file_system fsdesc_array[];
DEFINE_SYMBOL(fsdesc_array_end);

#endif

