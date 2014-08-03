#include "config.h"
#include "typedef.h"
#include "module.h"
#include "net.h"
#include "list.h"
#include "spinlock.h"
#include "memory.h"
#include "debug.h"
#include "ml_string.h"
#include "interrupt.h"
#include "task.h"

static LIST_HEAD(rxef_list);

static LIST_HEAD(valid_sock);


/* L2 */
static arpcache_t arpcache = {.count = 0, .listsize = 32};

int arp_insert(u8 *ipaddr, u8 *macaddr)
{
    ASSERT(ipaddr != 0);
    ASSERT(macaddr != 0);
    u32 count;

    /* each entry with ip match or mac match, should be remove */
    for (count = 0; count < arpcache.listsize; count++)
    {
        if ((arpcache.list[count].valid) &&
            (memcmp(arpcache.list[count].ip, ipaddr, 4) == 0))
        {
            arpcache.list[count].valid = 0;
        }
        if ((arpcache.list[count].valid) &&
            (memcmp(arpcache.list[count].mac, macaddr, 6) == 0))
        {
            arpcache.list[count].valid = 0;
        }
    }

    if (arpcache.count == arpcache.listsize)
    {
        printf("arp catch insert failed.\n");
        return -1;
    }

    /* get empty entry, insert it */
    for (count = 0; count < arpcache.listsize; count++)
    {
        if (arpcache.list[count].valid == 0)
        {
            arpcache.list[count].valid = 1;
            memcpy(arpcache.list[count].ip, ipaddr, 4);
            memcpy(arpcache.list[count].mac, macaddr, 6);
            return 0;
        }
    }

    /* shuold not reach here */
    ASSERT(0);
    return -1;
}

int arp_getmacaddr(u8 *ipaddr, u8 *macaddr)
{
    ASSERT(ipaddr != 0);
    u32 count;
    for (count = 0; count < arpcache.listsize; count++)
    {
        if ((arpcache.list[count].valid) &&
            (memcmp(arpcache.list[count].ip, ipaddr, 4) == 0))
        {
            if (macaddr)
                memcpy(macaddr, arpcache.list[count].mac, 6);
            return 0;
        }
    }
    return -1;
}

/* L3 */
static u8   ipaddr[4];
static u8   ipmask[4];
const static u8     ipaddr_any[4] = {0, 0, 0, 0};

u16 checksum(u32 cksum, void *pBuffer, u16 len)
{
    u8 num = 0;
    u8 *p = (u8 *)pBuffer;
 
    if ((NULL == p) || (0 == len))
    {
        return cksum;
    }
   
    while (len > 1)
    {
        cksum += ((u16)p[num] << 8 & 0xff00) | ((u16)p[num + 1] & 0x00FF);
        len   -= 2;
        num   += 2;
    }
   
    if (len > 0)
    {
        cksum += ((u16)p[num] << 8) & 0xFFFF;
        num += 1;
    }
 
    while (cksum >> 16)
    {
        cksum = (cksum & 0xFFFF) + (cksum >> 16);
    }
   
    return ~cksum;
}

void ipv4header_dump(ipv4_header_t *ipv4header)
{
    printf("ver:0x%1x, ihl:0x%1x, tos:0x%2x, total_len:%d\n"
           " id:0x%4x, flag:0x%1x, flag_off:0x%4x\n"
           "ttl:0x%2x, prot:0x%2x, checksum:0x%4x\n"
           "src ip: %d.%d.%d.%d "
           "dst ip: %d.%d.%d.%d\n",
           ipv4header->ver, ipv4header->headerlen, ipv4header->tos, ((u16)ipv4header->len_h) << 8 | ipv4header->len_l,
           ((u16)ipv4header->id_h) << 8 | ipv4header->id_l, ipv4header->flag, ((u16)ipv4header->fragoff_h5) << 8 | ipv4header->fragoff_l8,
           ipv4header->ttl, ipv4header->protocol, ((u16)ipv4header->checksum_h8) << 8 | ipv4header->checksum_l8,
           ipv4header->src_ip[0],
           ipv4header->src_ip[1],
           ipv4header->src_ip[2],
           ipv4header->src_ip[3],
           ipv4header->dst_ip[0],
           ipv4header->dst_ip[1],
           ipv4header->dst_ip[2],
           ipv4header->dst_ip[3]);
}

u16 getipv4totallen(ipv4_header_t *ipv4header)
{
    u16 ret = ipv4header->len_h;
    ret <<= 8;
    ret |= ipv4header->len_l;
    return ret;
}

void udpheader_dump(udp_header_t *udpheader)
{
    printf("srcport:%d  dstport:%d  len:%d\n",
           getudp_srcport(udpheader),
           getudp_dstport(udpheader),
           getudp_len(udpheader));
}

u16 getudp_srcport(udp_header_t *udpheader)
{
    u16 ret = udpheader->src_port_h;
    ret <<= 8;
    ret |= udpheader->src_port_l;
    return ret;
}

u16 getudp_dstport(udp_header_t *udpheader)
{
    u16 ret = udpheader->dst_port_h;
    ret <<= 8;
    ret |= udpheader->dst_port_l;
    return ret;
}

u16 getudp_len(udp_header_t *udpheader)
{
    u16 ret = udpheader->len_h;
    ret <<= 8;
    ret |= udpheader->len_l;
    return ret;
}

void ipv4_ipswap(ipv4_header_t *ipv4header)
{
    u8 tmp, count;
    for (count = 0; count < 4; count++)
    {
        tmp = ipv4header->src_ip[count];
        ipv4header->src_ip[count] = ipv4header->dst_ip[count];
        ipv4header->dst_ip[count] = tmp;
    }
}

void ethii_macswap(ethii_header_t *ethheader)
{
    u8 tmp, count;
    for (count = 0; count < 6; count++)
    {
        tmp = ethheader->srcaddr[count];
        ethheader->srcaddr[count] = ethheader->dstaddr[count];
        ethheader->dstaddr[count] = tmp;
    }

}

static void efproc(ethframe_t *ef)
{
    ethii_header_t *ethheader = (ethii_header_t *)(ef->buf);
    u16 l2headerlen = sizeof(ethii_header_t);
    u16 l3headerlen;

    /* 802.1q, if exist */
    if (SWAP_2B(ethheader->ethtype) == L2TYPE_8021Q)
    {
        /* now we don't support 802.1q */
        return;
    }
    
    switch (SWAP_2B(ethheader->ethtype))
    {
    case L2TYPE_ARP:
    {
        arp_header_t *arpheader = (arp_header_t *)((u32)ethheader + l2headerlen);

        if (SWAP_2B(arpheader->optype) == ARPOPTYPE_REQ)
        {
            if (memcmp(arpheader->dst_ipaddr, ipaddr, 4))
            {
                break;
            }
            
            /* let's save the arp info */
            arp_insert(arpheader->src_ipaddr, arpheader->src_macaddr);

            /* construct the reply pkt and send */
            arpheader->optype = SWAP_2B(ARPOPTYPE_ACK);
            memcpy(arpheader->dst_macaddr,
                   arpheader->src_macaddr,
                   6);
            memcpy(arpheader->dst_ipaddr,
                   arpheader->src_ipaddr,
                   4);

            memcpy(arpheader->src_macaddr, ef->netdev->macaddr, 6);
            memcpy(arpheader->src_ipaddr, ipaddr, 4);

            memcpy(ethheader->dstaddr, ethheader->srcaddr, 6);
            memcpy(ethheader->srcaddr, ef->netdev->macaddr, 6);

            ef->netdev->tx(ef->netdev, ethheader, 64);
        }
        break;
    }
    case L2TYPE_IPV4:
    {
        ipv4_header_t *ipv4header = (ipv4_header_t *)((u32)ethheader + l2headerlen);
        u16 cksum, l4len;

        /* check the ipv4 header checksum */
        l3headerlen = ipv4header->headerlen * 4;
        l4len = getipv4totallen(ipv4header) - l3headerlen;
        if (0 != checksum(0, ipv4header, l3headerlen))
        {
            return;
        }

        switch (ipv4header->protocol)
        {
        case IPV4PROTOCOL_ICMP:
        {
            icmp_header_t *icmpheader = (icmp_header_t *)((u32)ethheader + l2headerlen + l3headerlen);
            if (icmpheader->type == ICMPTYPE_ECHOREQ)
            {
                /* let's save the arp info */
                arp_insert(ipv4header->src_ip, ethheader->srcaddr);

                icmpheader->type = ICMPTYPE_ECHOACK;
                cksum = checksum(0, icmpheader, l4len);
                icmpheader->checksum_h8 = cksum >> 8;
                icmpheader->checksum_l8 = cksum & 0xFF;

                ipv4_ipswap(ipv4header);
                ethii_macswap(ethheader);

                ef->netdev->tx(ef->netdev, ethheader, ef->len);
            }
            break;
        }
        case IPV4PROTOCOL_UDP:
        {
            udp_header_t *udpheader = (udp_header_t *)((u32)ethheader + l2headerlen + l3headerlen);
            sock_context_t *sockctx;
            LIST_FOREACH_ELEMENT(sockctx, &valid_sock, list)
            {
                ASSERT(sockctx->sock_recv != 0);

                if (sockctx->protocol != IPV4PROTOCOL_UDP)
                    continue;
            
                /* check port */
                if ((sockctx->localport != PORT_ALL) &&
                    (sockctx->localport != getudp_dstport(udpheader)))
                    continue;

                if ((sockctx->destport != PORT_ALL) &&
                    (sockctx->destport != getudp_srcport(udpheader)))
                    continue;

                /* check ip */
                if ((memcmp(sockctx->localip, (void *)ipaddr_any, 4) != 0) &&
                    (memcmp(sockctx->localip, ipv4header->dst_ip, 4) != 0))
                    continue;

                if ((memcmp(sockctx->destip, (void *)ipaddr_any, 4) != 0) &&
                    (memcmp(sockctx->destip, ipv4header->src_ip, 4) != 0))
                    continue;

                /* ok, this sock can process this pkt */
                sockctx->netdev = ef->netdev;
                sockctx->sock_recv(sockctx,
                                   ethheader,
                                   ipv4header,
                                   udpheader,
                                   (void *)((u32)udpheader + sizeof(udp_header_t)),
                                   getudp_len(udpheader) - sizeof(udp_header_t));
            }

            break;
        }
        default:
            /* the protocol we don't support */
            break;
        }

        break;
    }
    default:
        break;
    }
}

/* pending frame procedure, process all of the rx ethframe */
int do_fbproc(void *param)
{
    ethframe_t *ef;
    struct list_head *first_ef;

    while (1)
    {
        if (CHECK_LIST_EMPTY(&rxef_list))
        {
            sleep(100);
            schedule();
            continue;
        }
 
        /* kick out and process the each ef */
        u32 flags;
        raw_local_irq_save(flags);
        first_ef = list_remove_head(&rxef_list);
        raw_local_irq_restore(flags);

        /* process the eth frame */
        ef = GET_CONTAINER(first_ef, ethframe_t, e);
        DEBUG("rx eth frame, len=%d\n", ef->len);
        
        efproc(ef);

        page_free(phyaddr2page(ef));
    }

    /* never reach here */
    return 0;
}

void rxef_insert(ethframe_t *ef)
{
    u32 flags;

    raw_local_irq_save(flags);
    list_add_tail(&(ef->e),&rxef_list);
    raw_local_irq_restore(flags);
}

/*  */
int sockctx_register(sock_context_t *sockctx)
{
    if (sockctx->sock_recv == 0)
    {
        return -1;
    }
    list_add_tail(&(sockctx->list), &valid_sock);

    return 0;
}

/* buff: just have upd data */
int udp_send(sock_context_t *sockctx, void *buff, u16 len)
{
    u16 cksum;
    ASSERT(sockctx->protocol == IPV4PROTOCOL_UDP);

    if (len < MINUDP_DATELEN)
    {
        len = MINUDP_DATELEN;
    }

    /* add upd header */
    len += sizeof(udp_header_t);
    upd_pseudoheader_t *pseudoupd = (upd_pseudoheader_t *)((u32)buff - sizeof(upd_pseudoheader_t));
    pseudoupd->udpheader.src_port_h = (u8)(sockctx->localport >> 8);
    pseudoupd->udpheader.src_port_l = (u8)(sockctx->localport >> 0);
    pseudoupd->udpheader.dst_port_h = (u8)(sockctx->destport >> 8);
    pseudoupd->udpheader.dst_port_l = (u8)(sockctx->destport >> 0);
    pseudoupd->udpheader.len_h = (u8)(len >> 8);
    pseudoupd->udpheader.len_l = (u8)(len >> 0);
    pseudoupd->udpheader.cksum_h = 0;
    pseudoupd->udpheader.cksum_l = 0;

    memcpy(pseudoupd->srcip, sockctx->localip, 4);
    memcpy(pseudoupd->dstip, sockctx->destip, 4);
    pseudoupd->rsv0 = 0;
    pseudoupd->protocol = IPV4PROTOCOL_UDP;
    pseudoupd->len_h = pseudoupd->udpheader.len_h;
    pseudoupd->len_l = pseudoupd->udpheader.len_l;

    cksum = checksum(0, pseudoupd, len + sizeof(upd_pseudoheader_t) - sizeof(udp_header_t));
    pseudoupd->udpheader.cksum_h = (u8)(cksum >> 8);
    pseudoupd->udpheader.cksum_l = (u8)(cksum >> 0);

    /* add ip header */
    len += sizeof(ipv4_header_t);
    ipv4_header_t *ipheader = (ipv4_header_t *)((u32)buff -
                                                sizeof(udp_header_t) -
                                                sizeof(ipv4_header_t));
    memset(ipheader, 0, 20);
    ipheader->ver = 4;
    ipheader->headerlen = 5;
    // ipheader->tos = ;
    ipheader->len_h = (u8)(len >> 8);
    ipheader->len_l = (u8)(len >> 0);

    // ipheader->id_h = ;
    // ipheader->id_l = ;
    ipheader->flag = 0x2;

    ipheader->ttl = 64;
    ipheader->protocol = IPV4PROTOCOL_UDP;
    ipheader->checksum_h8 = 0;
    ipheader->checksum_l8 = 0;

    memcpy(ipheader->src_ip, sockctx->localip, 4);
    memcpy(ipheader->dst_ip, sockctx->destip, 4);

    cksum = checksum(0, ipheader, 20);
    ipheader->checksum_h8 = (u8)(cksum >> 8);
    ipheader->checksum_l8 = (u8)(cksum >> 0);

    /* add eth header */
    len += sizeof(ethii_header_t);
    ethii_header_t *ethiiheader = (ethii_header_t *)((u32)ipheader - sizeof(ethii_header_t));
    if (arp_getmacaddr(sockctx->destip, ethiiheader->dstaddr) < 0)
    {
        printf("udp_send() failed, get arp_catch failed, dst ip:%d.%d.%d.%d.",
               sockctx->destip[0],
               sockctx->destip[1],
               sockctx->destip[2],
               sockctx->destip[3]);
        return -1;
    }
    memcpy(ethiiheader->srcaddr, sockctx->netdev->macaddr, 6);
    ethiiheader->ethtype = SWAP_2B(L2TYPE_IPV4);

    /* send the pkt */
    sockctx->netdev->tx(sockctx->netdev, ethiiheader, len);

    return 0;
}

static void __init netproc_init(void)
{
    u32 count;
    
    for (count = 0; count < arpcache.listsize; count++)
    {
        arpcache.list[count].valid = 0;
    }
    
    ipaddr[0] = 192;
    ipaddr[1] = 168;
    ipaddr[2] = 1;
    ipaddr[3] = 20;

    ipmask[0] = 255;
    ipmask[1] = 255;
    ipmask[2] = 255;
    ipmask[3] = 0;

    /* this the ethframe proc task */
    kernel_thread(do_fbproc, NULL);
}

module_init(netproc_init, 3);


