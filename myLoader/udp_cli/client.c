#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "config.h"
#include "typedef.h"
#include "udpcli.h"
#include "crc32.h"

u8 sendbuff[2048];
u8 recvbuff[2048];

static int updcli_hello(int sock)
{
    ssize_t size;
    size = send(sock, CONFIG_UDPCLI_HELLO_REQ, sizeof(CONFIG_UDPCLI_HELLO_REQ), 0);
    size = recv(sock, recvbuff, sizeof(recvbuff), 0);
    if (size <= 0)
    {
        printf("updcli_hello() size <= 0, size:%ld\n", size);
        return size;
    }
    recvbuff[size] = 0;
    if (memcmp(recvbuff, CONFIG_UDPCLI_HELLO_ACK, sizeof(CONFIG_UDPCLI_HELLO_ACK)))
    {
        printf("hello failed, server reply:%s\n", recvbuff);
        return -1;
    }

    printf("hello success.\n");

    return 0;
}

static int updcli_bye(int sock)
{
    ssize_t size;
    size = send(sock, CONFIG_UDPCLI_BYE_REQ, sizeof(CONFIG_UDPCLI_BYE_REQ), 0);
    size = recv(sock, recvbuff, sizeof(recvbuff), 0);
    if (size <= 0)
    {
        printf("updcli_bye() size <= 0, size:%ld\n", size);
        return size;
    }
    recvbuff[size] = 0;
    if (memcmp(recvbuff, CONFIG_UDPCLI_BYE_REQ, sizeof(CONFIG_UDPCLI_BYE_REQ)))
    {
        printf("bye, invalid ack, server reply:%s\n", recvbuff);
    }

    printf("bye. disconnect.\n");

    return 0;
}

static int udpcli_sendandwait(int sock, udpcli_cmd *cmd)
{
    udpcli_cmd ack;
    ssize_t size;
    int retry = 3;

udpcli_sendandwait_retry:

    cmd->cmd_ack = UDPCLI_CMDTYPE_REQ;

    size = send(sock, cmd, cmd->totallen, 0);
    size = recv(sock, &ack, sizeof(udpcli_cmd), 0);

    if (size <= 0)
    {
        printf("udpcli_sendandwait() size <= 0, size:%ld\n", size);
        if (retry)
        {
            retry--;
            goto udpcli_sendandwait_retry;
        }
        return size;
    }

    if ((ack.cmd_ack == UDPCLI_CMDTYPE_RETRY) &&
        (ack.cmdtype == cmd->cmdtype))
    {
        printf("need retrans.\n");
        if (retry)
        {
            retry--;
            goto udpcli_sendandwait_retry;
        }
        return -1;
    }

    if ((ack.cmd_ack != UDPCLI_CMDTYPE_ACK) ||
        (ack.cmdtype != cmd->cmdtype))
    {
        printf("command without ack. ack:%d, cmdtype:%d\n",
               ack.cmd_ack, ack.cmdtype);
        if (retry)
        {
            retry--;
            goto udpcli_sendandwait_retry;
        }
        return -1;
    }

    printf("udp trans success.\n");
    return 0;
}

/*
 * argv[0]: "sendfile"
 * argv[1]: "hd0" "hd1"
 * argv[2]: filename
 * argv[3]: 
 */
static int updcli_sendfile(int sock, int argc, char *argv[])
{
    u32 crc;
    udpcli_cmd *cmd = (udpcli_cmd *)malloc(sizeof(udpcli_cmd) + 2048);

    if (memcmp(argv[0], "sendfile", sizeof("sendfile")) == 0)
    {
        int fd = open(argv[2], O_RDONLY);
        if (fd < 0)
        {
            printf("file open failed, file:%s\n", argv[0]);
            return -1;
        }
        else
        {
            printf("file \"%s\" open success.", argv[0]);
        }
        u32 filelen, logicsect, logicsectoff;
        u8 *mappbuff;

        sscanf(argv[3], "%d", &logicsectoff);

        /* get file len */
        filelen = lseek(fd, 0, SEEK_END);
        printf("file len:%d   sector off:%d\n", filelen, logicsectoff);

        mappbuff = mmap(0, filelen, PROT_READ, MAP_PRIVATE, fd, 0);

        /* we just used the simplest protocol: stop and wait */
        cmd->cmdtype = UDPCLI_CMD_HDWRITE;
        cmd->totallen = sizeof(udpcli_cmd) + 1024 + 4;
        if (memcmp(argv[1], "hd0", sizeof("hd0")) == 0)
        {
            cmd->u.hdwrite_s.hdidx = 0;
        }
        else if (memcmp(argv[1], "hd1", sizeof("hd1")) == 0)
        {
            cmd->u.hdwrite_s.hdidx = 1;
        }

        for (logicsect = 0; logicsect < ((filelen + 1023) / 1024); logicsect++)
        {
            cmd->id = logicsect;
            cmd->u.hdwrite_s.logicsect = logicsect + logicsectoff;
            
            memcpy(cmd->buff, mappbuff + logicsect * 1024, 1024);
            crc = crc32(cmd->buff, 1024);
            cmd->buff[1024] = (u8)(crc >> 24);
            cmd->buff[1025] = (u8)(crc >> 16);
            cmd->buff[1026] = (u8)(crc >> 8);
            cmd->buff[1027] = (u8)(crc >> 0);
            if (0 != crc32_raw(cmd->buff, cmd->totallen - sizeof(udpcli_cmd)))
            {
                printf("crc self check failed.\n");
            }
            printf("send pkt...\n");
            udpcli_sendandwait(sock, cmd);
        }
    }
    else
    {
        printf("unknow argv:%s\n", argv[0]);
    }

    return 0;
}

/* argv[1]: ip   x.x.x.x
 * argv[2]: sendfile
 * argv[3]: hd0, hd1
 * argv[4]: filename
 * argv[5]: sectoff
 */
int main(int argc, char *argv[])
{
    int clientsock = CONFIG_UDPCLI_DEFAULTPORT;
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;

    if (argc != 6)
    {
        printf("invalid param count. exit.\n");
        exit(0);
    }

    /* get and config the server ip:port */
    if (inet_pton(AF_INET, argv[1], &serveraddr.sin_addr) <= 0)
    {
        printf("[%s] is not a valid IPaddress\n", argv[1]);
        exit(0);
    }
    sscanf(argv[2], "%d", &clientsock);
    serveraddr.sin_port = htons((u16)clientsock);


    printf("prepare to connect server:%d.%d.%d.%d %d\n",
           (u8)(serveraddr.sin_addr.s_addr >> 0),
           (u8)(serveraddr.sin_addr.s_addr >> 8),
           (u8)(serveraddr.sin_addr.s_addr >> 16),
           (u8)(serveraddr.sin_addr.s_addr >> 24),
           ntohs(serveraddr.sin_port));


    /* open socket */
    clientsock = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientsock < 0)
    {
        printf("socket create failed. exit.\n");
        exit(0);
    }

    /* connect to server */
    socklen_t addrlen = sizeof(serveraddr);
    if (connect(clientsock, (struct sockaddr *)(&serveraddr), addrlen) == -1)
    {
        perror("connect error");
        exit(1);
    }

    /* udpcli begin */
    updcli_hello(clientsock);

    
    updcli_sendfile(clientsock, argc - 2, argv + 2);


    /* udpcli end */
    updcli_bye(clientsock);


    close(clientsock);
    
    return 0;
}

