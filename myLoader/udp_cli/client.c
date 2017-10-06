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

	*cmd = ack;

    return 0;
}

/*
 * argv[0]: "hd0" "hd1"
 * argv[1]: filename
 * argv[2]: sector offset
 */
static int updcli_hdwrite(int sock, int argc, char *argv[])
{
    u32 crc;
    udpcli_cmd *cmd = (udpcli_cmd *)malloc(sizeof(udpcli_cmd) + 2048);

    if (argc != 3)
    {
        printf("invalid param count. exit.\n");
        exit(0);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        printf("file open failed, file:%s\n", argv[1]);
        return -1;
    }
    else
    {
        printf("file \"%s\" open success.", argv[1]);
    }
    u32 filelen, logicsect, logicsectoff;
    u8 *mappbuff;

    sscanf(argv[2], "%d", &logicsectoff);

    /* get file len */
    filelen = lseek(fd, 0, SEEK_END);
    printf("file len:%d   sector off:%d\n", filelen, logicsectoff);

    mappbuff = mmap(0, filelen, PROT_READ, MAP_PRIVATE, fd, 0);

    /* we just used the simplest protocol: stop and wait */
    cmd->cmdtype = UDPCLI_CMD_HDWRITE;
    cmd->totallen = sizeof(udpcli_cmd) + 1024 + 4;
    if (memcmp(argv[0], "hd0", sizeof("hd0")) == 0)
    {
        cmd->u.hdwrite_s.hdidx = 0;
    }
    else if (memcmp(argv[0], "hd1", sizeof("hd1")) == 0)
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

    return 0;
}

/*
 * argv[0]: filename
 * argv[1]: [addr]
 */
static int updcli_ramwrite(int sock, int argc, char *argv[])
{
	udpcli_cmd *cmd = (udpcli_cmd *)malloc(sizeof(udpcli_cmd) + 2048);
    u32 i, left, filelen, destaddr, crc;
    u8 *mappbuff;

	/* read file */
	int fd = open(argv[0], O_RDONLY);
    if (fd < 0)
    {
        printf("file open failed, file:%s\n", argv[0]);
        return -1;
    }
    else
    {
        printf("file \"%s\" open success.", argv[0]);
    }
    /* get file len */
    filelen = lseek(fd, 0, SEEK_END);
    printf("file len:%d\n", filelen);
	/* map file */
	mappbuff = mmap(0, filelen, PROT_READ, MAP_PRIVATE, fd, 0);


	if (argc == 1)
	{
		/* need send ramwrite_pre */
		cmd->cmdtype = UDPCLI_CMD_RAMWRITE_PRE;
		cmd->u.ramwrite_s.size = filelen;
		cmd->totallen = sizeof(udpcli_cmd);
		if (udpcli_sendandwait(sock, cmd) != 0)
			exit(0);

		destaddr = cmd->u.ramwrite_s.addr;
		printf("ram write pre success.\n");
	}

	else if (argc != 2)
	{
        printf("invalid param count. exit.\n");
        exit(0);
	}
	else
	{
		sscanf(argv[1], "0x%x", &destaddr);
	}

	printf("ram write dest addr: 0x%08x\n", destaddr);

	left = filelen;
	i = 0;
    while (left)
    {
    	cmd->cmdtype = UDPCLI_CMD_RAMWRITE;
    	cmd->totallen = sizeof(udpcli_cmd) + 1024 + 4;
        cmd->u.ramwrite_s.addr = destaddr + i * 1024;
		cmd->u.ramwrite_s.size = left < 1024 ? left : 1024;

        memcpy(cmd->buff, mappbuff + i * 1024, cmd->u.ramwrite_s.size);
        crc = crc32(cmd->buff, 1024);
        cmd->buff[1024] = (u8)(crc >> 24);
        cmd->buff[1025] = (u8)(crc >> 16);
        cmd->buff[1026] = (u8)(crc >> 8);
        cmd->buff[1027] = (u8)(crc >> 0);

		left -= cmd->u.ramwrite_s.size;
		i++;

        if (udpcli_sendandwait(sock, cmd) == 0)
			printf("\r%%%2.2f", (filelen - left) * 100.0 / filelen);
    }

	free(cmd);

	return 0;
}

/* argv[1]: ip   x.x.x.x
 * argv[2]: hdwrite
 * argv[3]: hd0 | hd1
 * argv[4]: filename
 * argv[5]: sectoff
 */

/* argv[1]: ip   x.x.x.x
 * argv[2]: ramwrite
 * argv[3]: filename
 * argv[4]: [addr]
 */
int main(int argc, char *argv[])
{
    int clientsock = CONFIG_UDPCLI_DEFAULTPORT;
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;

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


	if (memcmp(argv[2], "hdwrite", sizeof("hdwrite")) == 0)
	{
		updcli_hdwrite(clientsock, argc - 3, argv + 3);
	}
	else if (memcmp(argv[2], "ramwrite", sizeof("ramwrite")) == 0)
	{
		updcli_ramwrite(clientsock, argc - 3, argv + 3);
	}
    else
    {
        printf("unknow argv:%s\n", argv[2]);
    }


    /* udpcli end */
    updcli_bye(clientsock);


    close(clientsock);

    return 0;
}

