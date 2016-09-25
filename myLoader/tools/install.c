#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "crc32.h"
#include "boot.h"


/*
 * argv[1]: sys.img or sys.iso
 * argv[2]: boot.bin
 * argv[3]: core.bin
 * argv[4]: -iso -disk
 */
int main(int argc, char *argv[])
{
    int fd_sys, fd_boot, fd_core, core_size, sys_size, core_off;
    struct stat stat_str;
    struct bootparam *bootp;
    void *boot, *core, *sys, *dstpos;


    /* prepare the boot */
    fd_boot = open(argv[2], O_RDONLY);
    if (fd_boot < 0)
    {
        printf("can't open the \"bootbin\" file: %s", argv[2]);
        exit(0);
    }
    boot = mmap(NULL, 512, PROT_READ, MAP_SHARED, fd_boot, 0);
    if (boot == MAP_FAILED)
    {
        close(fd_boot);
        printf("can't map the \"bootbin\" file: %s", argv[2]);
        exit(0);
    }

    /* prepare the core */
    fd_core = open(argv[3], O_RDONLY);
    if (fd_core < 0)
    {
        close(fd_boot);
        printf("can't open the \"corebin\" file: %s", argv[3]);
        exit(0);
    }
	if (fstat(fd_core, &stat_str))
    {   
        close(fd_boot);
        close(fd_core);
		printf("Unable to stat `%s': %m", argv[3]);
        exit(0);
    }
    core_size = (stat_str.st_size + 511) & (~511);		/* last page is the .coreentry.param */
	printf("core size: %u Bytes, %u sectors.\n", core_size, core_size / 512);

    core = mmap(NULL, stat_str.st_size, PROT_READ, MAP_SHARED, fd_core, 0);
    if (core == MAP_FAILED)
    {
        close(fd_boot);
        close(fd_core);
        printf("can't map the \"corebin\" file: %s", argv[3]);
        exit(0);
    }

    /* prepare the sys */
    fd_sys = open(argv[1], O_RDWR);
    if (fd_sys < 0)
    {
        close(fd_boot);
        close(fd_core);
        printf("can't open the \"sys\" file: %s", argv[1]);
        exit(0);
    }
	if (fstat(fd_sys, &stat_str))
    {   
        close(fd_boot);
        close(fd_core);
		close(fd_sys);
		printf("Unable to stat `%s': %m", argv[1]);
        exit(0);
    }
	sys_size = stat_str.st_size;
	printf("sys size: %d Bytes.\n", sys_size);

    sys = mmap(NULL, stat_str.st_size, PROT_WRITE, MAP_SHARED, fd_sys, 0);
    if (sys == MAP_FAILED)
    {
        close(fd_boot);
        close(fd_core);
        close(fd_sys);
        printf("can't map the \"sys\" file: %s", argv[1]);
        exit(0);
    }

	if (memcmp(argv[4], "-disk", strlen(argv[4])) == 0)
	{
		/* make disk image */
		dstpos = sys;

		core_off = 1;
	}
	else if (memcmp(argv[4], "-iso", strlen(argv[4])) == 0)
	{
		/* we need to find the image start pos in iso */
		unsigned lba = 0x11, sz_sect = 0x800;
		unsigned char *checkb;
		lba = *((unsigned *)(sys + lba * sz_sect + 0x47));
		checkb = (unsigned char *)(sys + lba * sz_sect);
		if ((checkb[0] != 0x01) ||
			(checkb[1] != 0x00) ||
			(checkb[0x1E] != 0x55) ||
			(checkb[0x1F] != 0xAA))
		{
			printf("ISO file check flag broken.\n");
			goto main_ret;
		}
		lba = *((unsigned *)(sys + lba * sz_sect + 0x28));
		dstpos = sys + lba * sz_sect;
		printf("boot begin pos in ISO: 0x%08X.\n", (unsigned)(dstpos - sys));

		core_off = 4;
	}
	else
	{
		printf("invalid param \"%s\".\n", argv[4]);
		goto main_ret;
	}
	
	memset(dstpos + 512, 0, core_size);
	
    memcpy(dstpos, boot, 512 - sizeof(struct bootparam));
    bootp = (struct bootparam *)((unsigned long)dstpos + (512 - sizeof(struct bootparam)));
	memset(bootp, 0, sizeof(struct bootparam) - 4 * sizeof(struct hd_dptentry) - 2);
    bootp->n_sect = core_size / 512;
    bootp->bootsectflag = 0xAA55;

	bootp->core_lba = ((unsigned long)dstpos - (unsigned long)sys) / 512 + core_off;
	printf("core lba: 0x%08x\n", bootp->core_lba);

	/* copy the core.bin */
    memcpy((void *)((unsigned long)dstpos + 512 * core_off), core, core_size);

	/* let's calc the CRC */
	bootp->core_crc = crc32((void *)((unsigned long)core + 0x3000), core_size - 0x3000);
	printf("core crc:0x%8x\n", bootp->core_crc);

main_ret:
    close(fd_sys);
    close(fd_core);
    close(fd_boot);
    return 0;
}

