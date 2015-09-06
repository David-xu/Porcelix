#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "crc32.h"

int main(int argc, char *argv[])
{
    int infd;
    char *buf;
    int len, r;
    if (argc != 2)
    {
        printf("valid param num.\n");
        exit(0);
    }
    else
    {
        infd = open((const char *)argv[1], O_RDONLY, 0);
        if (infd < 0)
        {
            printf("invalid input file. exit.\n");
            exit(0);
        }
        len = lseek(infd, 0, SEEK_END);
    }

    buf = mmap(0, len, PROT_READ, MAP_SHARED, infd, 0);
#if 1
    r = crc32(buf, len);
    printf("reg2:0x%8x\n", r);
#else
    char r_buf[8] = {1, 0, 0, 0, 0xdc, 0x6d, 0x9a, 0xb7};
    r = crc32_raw(r_buf, sizeof(r_buf));
    printf("reg2:0x%8x\n", r);
#endif

    close(infd);

    return 0;
}
