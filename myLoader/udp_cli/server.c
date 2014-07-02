#include "config.h"
#include "typedef.h"
#include "net.h"
#include "module.h"
#include "string.h"
#include "command.h"
#include "udpcli.h"
#include "device.h"
#include "hd.h"
#include "debug.h"
#include "crc32.h"

static void udpcli_reset(sock_context_t *sock_ctx)
{
    sock_ctx->localport = CONFIG_UDPCLI_DEFAULTPORT;
    sock_ctx->destport = PORT_ALL;
    memset(sock_ctx->localip, 0, 4);
    memset(sock_ctx->destip, 0, 4);

    printf("[udpcli]reset\n");
}

/* buff: upd date(without any header) */
static int udpcli_recv
(
    sock_context_t *sock_ctx,
    void *l2header,
    void *l3header,
    void *l4header,
    void *date,
    u16 len
)
{
    ipv4_header_t *ipv4header = (ipv4_header_t *)l3header;
    udp_header_t *updheader = (udp_header_t *)l4header;

    /*  */
    if (memcmp(date, CONFIG_UDPCLI_HELLO_REQ, sizeof(CONFIG_UDPCLI_HELLO_REQ)) == 0)
    {
        /* connet from the client */
        printf("\n[udpcli]get connect from client:%d.%d.%d.%d %d\n",
               ipv4header->src_ip[0],
               ipv4header->src_ip[1],
               ipv4header->src_ip[2],
               ipv4header->src_ip[3],
               getudp_srcport(updheader));

        sock_ctx->destport = getudp_srcport(updheader);
        memcpy(sock_ctx->localip, ipv4header->dst_ip, 4);
        memcpy(sock_ctx->destip, ipv4header->src_ip, 4);

        /* send reply */
        len = sizeof(CONFIG_UDPCLI_HELLO_ACK);
        memcpy(date, CONFIG_UDPCLI_HELLO_ACK, len);
        udp_send(sock_ctx, date, len);
    }
    else if (memcmp(date, CONFIG_UDPCLI_BYE_REQ, sizeof(CONFIG_UDPCLI_BYE_REQ)) == 0)
    {    
        /* disconnect from the client */
        printf("[udpcli]disconnect from client:%d.%d.%d.%d %d\n",
               sock_ctx->destip[0],
               sock_ctx->destip[1],
               sock_ctx->destip[2],
               sock_ctx->destip[3],
               sock_ctx->destport);

        /* send reply */
        len = sizeof(CONFIG_UDPCLI_BYE_REQ);
        memcpy(date, CONFIG_UDPCLI_BYE_REQ, len);
        udp_send(sock_ctx, date, len);

        /* reset udpcli */
        udpcli_reset(sock_ctx);
    }
    else
    {
        udpcli_cmd *cmd = (udpcli_cmd *)date;
        if (cmd->cmd_ack != UDPCLI_CMDTYPE_REQ)
        {
            printf("[udpcli]need req command.\n");
            dump_ram(cmd, 32);
            return 0;
        }
        if (cmd->cmdtype == UDPCLI_CMD_HDWRITE)
        {
            /* check the crc */
            if (0 != crc32_raw(cmd->buff, cmd->totallen - sizeof(udpcli_cmd)))
            {
                printf("crc check failed.\n");
                cmd->cmd_ack = UDPCLI_CMDTYPE_RETRY;
            }
            else
            {
                device_t *hd_dev = 0;
                if (cmd->u.hdwrite_s.hdidx == 0)
                {
                    hd_dev = gethd_dev(HDPART_HD0_WHOLE);
                }
                else if (cmd->u.hdwrite_s.hdidx == 1)
                {
                    hd_dev = gethd_dev(HDPART_HD1_WHOLE);
                }
                else
                {
                    printf("invalid hd idx:%d\n", cmd->u.hdwrite_s.hdidx);
                }
                hd_dev->driver->write(hd_dev,
                                      cmd->u.hdwrite_s.logicsect,
                                      cmd->buff,
                                      1);
                printf("[udpcli]hd write finished, logicsector:%d\n", cmd->u.hdwrite_s.logicsect);
                cmd->cmd_ack = UDPCLI_CMDTYPE_ACK;
            }
            cmd->totallen = sizeof(udpcli_cmd);

            udp_send(sock_ctx, cmd, sizeof(udpcli_cmd));
        }
        else
        {
            printf("[udpcli]unknown command type:%d\n", cmd->cmdtype);
        }
    }
    
    return 0;
}

/* leave the srcip == 0.0.0.0 */
static sock_context_t udpcli_sockctx = 
{
    .localport = CONFIG_UDPCLI_DEFAULTPORT,
    .destport = PORT_ALL,
    .sock_recv = udpcli_recv,
    .protocol = IPV4PROTOCOL_UDP,
};

static void __init udpcli_init(void)
{
    udpcli_reset(&udpcli_sockctx);
    sockctx_register(&udpcli_sockctx);
}

module_init(udpcli_init, 7);

static void cmd_udpclireset_opfunc(char *argv[], int argc, void *param)
{
    udpcli_reset(&udpcli_sockctx);
}

struct command cmd_udpclireset _SECTION_(.array.cmd) =
{
    .cmd_name   = "udpcli_reset",
    .info       = "reset the udpcli.",
    .op_func    = cmd_udpclireset_opfunc,
};



