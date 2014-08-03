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
 * argv[1]: sys.img
 * argv[2]: boot.bin
 * argv[3]: core.bin
 */
int main(int argc, char *argv[])
{
    int fd_sys, fd_boot, fd_core, core_size;
    struct stat sb_core;
    struct bootparam *bootp;
    void *boot, *core, *sys;


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
	if (fstat(fd_core, &sb_core))
    {   
        close(fd_boot);
        close(fd_core);
		printf("Unable to stat `%s': %m", argv[3]);
        exit(0);
    }
    core_size = (sb_core.st_size + 511) & (~511);		/* last page is the .coreentry.param */
	printf("core size: %u Bytes, %u sectors.\n", core_size, core_size / 512);

    core = mmap(NULL, sb_core.st_size, PROT_READ, MAP_SHARED, fd_core, 0);
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
    sys = mmap(NULL, 512 + core_size, PROT_WRITE, MAP_SHARED, fd_sys, 0);
    if (sys == MAP_FAILED)
    {
        close(fd_boot);
        close(fd_core);
        close(fd_sys);
        printf("can't map the \"sys\" file: %s", argv[1]);
        exit(0);
    }
	memset(sys + 512, 0, core_size);
	
    memcpy(sys, boot, 512 - sizeof(struct bootparam));
    bootp = (struct bootparam *)((unsigned)sys + (512 - sizeof(struct bootparam)));
    printf("addr 0x%x, write bootsect param:\n", (unsigned)bootp - (unsigned)sys);
    bootp->n_sect = core_size / 512;
    bootp->bootsectflag = 0xAA55;

	/* copy the core.bin */
    memcpy((void *)((unsigned)sys + 512), core, core_size);

	/* let's calc the CRC */
	bootp->core_crc = crc32((void *)((unsigned)sys + 512 + 8192), core_size - 8192);
	printf("core crc:0x%8x\n", bootp->core_crc);

    close(fd_sys);
    close(fd_core);
    close(fd_boot);
    return 0;
}

