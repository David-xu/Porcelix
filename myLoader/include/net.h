#ifndef _NET_H_
#define _NET_H_

#include "typedef.h"
#include "list.h"


struct pci_dev;
typedef struct _netdev {
    struct pci_dev *dev;

    u16     txpktidx;
    u16     rx_cur;

    void    *rxDMA;
    void    *txDMA;
    u8      macaddr[6];         /* device mac */

    int (*tx)(struct _netdev *netdev, void *buf, u32 len);
} netdev_t;

typedef struct _ethframe {
    struct list_head e;
    netdev_t *netdev;
    u16 len;
    u8 *buf;
    u8 bufs[0];
} ethframe_t;

#define L2TYPE_8021Q        (0x8100)
#define L2TYPE_ARP          (0x0806)
#define L2TYPE_IPV4         (0x0800)
typedef struct _ethii_header {
    u8  dstaddr[6];
    u8  srcaddr[6];

    u16 ethtype;
} __attribute__((packed)) ethii_header_t;


#define     ARPOPTYPE_REQ       (1)
#define     ARPOPTYPE_ACK       (2)
typedef struct _arp_header {
    u16     hwtype;
    u16     prottype;
    u8      hwaddr_len;
    u8      protaddr_len;

    u16     optype;

    u8      src_macaddr[6];
    u8      src_ipaddr[4];
    u8      dst_macaddr[6];
    u8      dst_ipaddr[4];
} __attribute__((packed)) arp_header_t;

#define     IPV4PROTOCOL_ICMP   (1)
#define     IPV4PROTOCOL_TCP    (6)
#define     IPV4PROTOCOL_UDP    (17)
typedef struct _ipv4_header {
    u8      headerlen   : 4;
    u8      ver         : 4;
    u8      tos;
    u8      len_h, len_l;

    u8      id_h, id_l;
    u8      fragoff_h5  : 5;
    u8      flag        : 3;
    u8      fragoff_l8;

    u8      ttl;
    u8      protocol;
    u8      checksum_h8, checksum_l8;

    u8  src_ip[4];
    u8  dst_ip[4];
} __attribute__((packed)) ipv4_header_t;

typedef struct _arpcache {
    struct {
        u8 ip[4];
        u8 mac[6];
        u16 valid;
    } list[32];
    u32 count, listsize;
} arpcache_t;

int arp_insert(u8 *ipaddr, u8 *macaddr);
int arp_getmacaddr(u8 *ipaddr, u8 *macaddr);

#define     ICMPTYPE_ECHOREQ    (8)
#define     ICMPTYPE_ECHOACK    (0)
typedef struct _icmp_header {
    u8      type;
    u8      code;
    u8      checksum_h8, checksum_l8;

    u8      id_h, id_l;
    u8      seq_h, seq_l;
} __attribute__((packed)) icmp_header_t;

#define MINUDP_DATELEN          (18)
typedef struct _udp_header {
    u8      src_port_h, src_port_l;
    u8      dst_port_h, dst_port_l;

    u8      len_h, len_l;
    u8      cksum_h, cksum_l;
} __attribute__((packed)) udp_header_t;

typedef struct _upd_pseudoheader {
    u8  srcip[4];
    u8  dstip[4];
    u8  rsv0, protocol;
    u8  len_h, len_l;       /* eq to the same field of the updheader */
    udp_header_t udpheader;
} __attribute__((packed)) upd_pseudoheader_t;

/*
typedef struct _ethii_vlantag {
    
} ethii_vlantag;
*/

/* do pending frame procedure */
void do_fbproc(void);

void rxef_insert(ethframe_t *ef);

#define PORT_ALL            (0xFFFF)
typedef struct _sock_context {
    struct list_head list;
    u16     localport, destport;
    u8      localip[4], destip[4];
    netdev_t    *netdev;

    /* engine will call this func */
    int (*sock_recv)(struct _sock_context *sock_ctx,
                     void *l2header,
                     void *l3header,
                     void *l4header,
                     void *date,
                     u16 len);          /* just date */

    u8      protocol;
} sock_context_t;

int sockctx_register(sock_context_t *sockctx);

int udp_send(sock_context_t *sockctx, void *buff, u16 len);

u16 checksum(u32 cksum, void *pBuffer, u16 len);
void ipv4header_dump(ipv4_header_t *ipv4header);
u16 getipv4totallen(ipv4_header_t *ipv4header);
void udpheader_dump(udp_header_t *udpheader);
u16 getudp_srcport(udp_header_t *udpheader);
u16 getudp_dstport(udp_header_t *udpheader);
u16 getudp_len(udp_header_t *udpheader);
void ipv4_ipswap(ipv4_header_t *ipv4header);
void ethii_macswap(ethii_header_t *ethheader);


#endif

