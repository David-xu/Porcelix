#include "config.h"
#include "typedef.h"
#include "public.h"
#include "section.h"
#include "module.h"
#include "interrupt.h"
#include "pci.h"
#include "memory.h"
#include "debug.h"
#include "net.h"
#include "spinlock.h"
#include "ml_string.h"
#include "command.h"

/* some pci vendor device */
#define PCI_VENDOR_ID_REALTEK		0x10ec
#define PCI_DEVICE_ID_REALTEK_8139	0x8139

/* reg offset */
#define PCI_RTL8139_TSD0            (0x0010)        /* 4Byte * 4discraptor */
#define PCI_RTL8139_TSAD0           (0x0020)        /* 4Byte * 4discraptor */
#define PCI_RTL8139_RBSTART_OFFSET  (0x0030)        /* 4Byte */
#define PCI_RTL8139_CR_OFFSET       (0x0037)        /* 1Byte */
#define PCI_RTL8139_CAPR_OFFSET     (0x0038)        /* 2Byte: Current Address of Packet Read */
#define PCI_RTL8139_CBR_OFFSET      (0x003a)        /* 2Byte: Current Buffer Address */
#define PCI_RTL8139_IMR_OFFSET      (0x003C)        /* 2Byte */
#define PCI_RTL8139_ISR_OFFSET      (0x003E)        /* 2Byte */
#define PCI_RTL8139_TCR_OFFSET      (0x0040)        /* 4Byte */
#define PCI_RTL8139_RCR_OFFSET      (0x0044)        /* 4Byte */
#define PCI_RTL8139_CFG93C46_OFFSET (0x0050)        /* 1Byte */
#define PCI_RTL8139_PCIREV_OFFSET   (0x005E)        /* 1Byte */
#define PCI_RTL8139_CSCR_OFFSET     (0x0074)        /* 2Byte */

/* Interrupt register bits, using my own meaningful names. */
enum IntrStatusBits {
	PCIErr		= 0x8000,
	PCSTimeout	= 0x4000,
	RxFIFOOver	= 0x40,
	RxUnderrun	= 0x20,
	RxOverflow	= 0x10,
	TxErr		= 0x08,
	TxOK		= 0x04,
	RxErr		= 0x02,
	RxOK		= 0x01,

	RxAckBits	= RxFIFOOver | RxOverflow | RxOK,
};

enum Cfg9346Bits {
	Cfg9346_Lock	= 0x00,
	Cfg9346_Unlock	= 0xC0,
};


static u32 rtl8139_intstat[16] = {0};

static u32 rtl8139_rxdbg = 0;
static u32 rtl8139_txdbg = 0;


#define RX_BUF_LEN          2048
static void realtek8139_isr(struct pt_regs *regs, void *param)
{
    struct pci_dev *dev = (struct pci_dev *)param;
    netdev_t *rtk8139 = (netdev_t *)(dev->param);
    u16 isr = inw(dev->bar_io + PCI_RTL8139_ISR_OFFSET);
    DEBUG("realtek8139_isr... isr:%#4x\n", isr);
{
    u32 count, flag = 0x1;
    for (count = 0; count < 16; count++)
    {
        if (flag & isr)
        {
            rtl8139_intstat[count]++;
        }
        flag <<= 1;
    }
}
    /* rx int */
    if (isr & RxOK)
    {
        while ((inb(dev->bar_io + PCI_RTL8139_CR_OFFSET) & 0x01) == 0)
        {
            ethframe_t *new_ef = (ethframe_t *)page_alloc(BUDDY_RANK_4K, MMAREA_NORMAL);
            u32 rxstatus = *((u32 *)((u32)rtk8139->rxDMA + rtk8139->rx_cur));

            new_ef->netdev = rtk8139;
            new_ef->len = (u16)(rxstatus >> 16);
            new_ef->buf = new_ef->buf + 128;    /*  */
            memcpy(new_ef->buf, (void *)((u32)(rtk8139->rxDMA) +
                                          rtk8139->rx_cur + 4), new_ef->len);
            if (rtl8139_rxdbg)
            {
                printf("rxstatus:0x%#8x  "
                       "CAPR:0x%#4x   CBR:0x%#4x\n"
                       "rx pktdump:\n",
                       rxstatus,
                       inw(dev->bar_io + PCI_RTL8139_CAPR_OFFSET),
                       inw(dev->bar_io + PCI_RTL8139_CBR_OFFSET));
                dump_ram(new_ef->buf, new_ef->len);
                rtl8139_rxdbg--;
            }

            rxef_insert(new_ef);

            rtk8139->rx_cur += ((new_ef->len + 4 + 3) & ~0x3);
            rtk8139->rx_cur %= 8192;
            
            outw(rtk8139->rx_cur - 16, dev->bar_io + PCI_RTL8139_CAPR_OFFSET);
        }

        /* clear the rx int */
        outw(RxOK, dev->bar_io + PCI_RTL8139_ISR_OFFSET);
    }
    else
    {
        /* clear all other int */
        outw(isr, dev->bar_io + PCI_RTL8139_ISR_OFFSET);
    }
}

static int realtek8139_tx(netdev_t *netdev, void *buf, u32 len)
{
    u32 desc_idx = (netdev->txpktidx) % 4;
    if (len < 60)
        len = 60;

    if (rtl8139_txdbg)
    {
        printf("tx pktdump:\n");
        dump_ram(buf, len);

        ethii_header_t *ethiiheader = (ethii_header_t *)buf;
        if (SWAP_2B(ethiiheader->ethtype) == L2TYPE_IPV4)
        {
            printf("ipv4 header:\n");
            ipv4_header_t *ipv4header = (ipv4_header_t *)((u32)buf + sizeof(ethii_header_t));
            ipv4header_dump(ipv4header);

            if (ipv4header->protocol == IPV4PROTOCOL_UDP)
            {
                printf("udp header:\n");
                udp_header_t *udpheader = (udp_header_t *)((u32)ipv4header + ipv4header->headerlen * 4);
                udpheader_dump(udpheader);
            }
        }
        

        rtl8139_txdbg--;
    }

    /* copy the data into DMA space */
    memcpy((void *)((u32)netdev->txDMA + desc_idx * 2048),
           buf,
           len);

    /* set the tx status register to start the transmit */
    outl(0x20000 | len,
         netdev->dev->bar_io + PCI_RTL8139_TSD0 + 4 * desc_idx);

    (netdev->txpktidx)++;
    
    return 0;
}

static int realtek8139_devinit(struct pci_dev *dev)
{
    u32 count, val;
    netdev_t *rtk8139;
    
    /* get the bar of I/O and bar of MEM */
    for (count = 0; count < ARRAY_ELEMENT_SIZE(dev->pcicfg.bar_mask); count++)
    {
        if ((dev->pcicfg.bar_mask[count]) == 0)
        {
            continue;
        }
        /* I/O BAR */
        if ((dev->pcicfg.u.cfg.baseaddr[count]) & 0x1)
        {
            dev->bar_io = (u16)(dev->pcicfg.u.cfg.baseaddr[count] & dev->pcicfg.bar_mask[count]);
        }
        /* MEM BAR */
        else
        {
            dev->bar_mem = dev->pcicfg.u.cfg.baseaddr[count] & dev->pcicfg.bar_mask[count];
        }
    }

    /* register the int */
    if (interrup_register(dev->pcicfg.u.cfg.intline, realtek8139_isr, dev) != 0)
    {
        printf("realtek8139_isr register failed.\n");
        return -1;
    }

    rtk8139 = (netdev_t *)(dev + 1);
    rtk8139->dev = dev;
    rtk8139->txpktidx = 0;
    rtk8139->rx_cur = 0;
    rtk8139->rxDMA = page_alloc(BUDDY_RANK_8K, MMAREA_LOW1M);			/* 8K in low 1M, used by DMA */
    rtk8139->txDMA = page_alloc(BUDDY_RANK_8K, MMAREA_LOW1M);			/* 2K * 4 in low 1M, used by DMA */
    rtk8139->tx = realtek8139_tx;
    dev->param = rtk8139;

    /* cfg93c46 unlock */
    outb(Cfg9346_Unlock, dev->bar_io + PCI_RTL8139_CFG93C46_OFFSET);

    count = inl(dev->bar_io);
    memcpy(rtk8139->macaddr, &count, 4);
    count = inl(dev->bar_io + 4);
    memcpy(rtk8139->macaddr + 4, &count, 2);

    /* 1.set the RBSTART(recive buffer start addr) */
    val = (u32)(rtk8139->rxDMA);
    outl(val, dev->bar_io + PCI_RTL8139_RBSTART_OFFSET);

    /* 2.set the CR(command register) */
	/* Must enable Tx/Rx before setting transfer thresholds! */
    outb(0x0c, dev->bar_io + PCI_RTL8139_CR_OFFSET);

    /* 3.set the RCR(recive config register) */
    val = inl(dev->bar_io + PCI_RTL8139_RCR_OFFSET);
    val |= (0x0 & 0x3) << 11;           /* RBLEN:bit[12:11] == 2b'00,  8K+16B */
    val |= (0x6 & 0x7) << 8;            /* MXDMA:bit[10:8]  == 3b'110, 1024B */
    val |= (0x2 & 0x7) << 13;           /* RXFTH:bit[15:13] == 3b'010, 64B*/
    val |= 0xE;                         /* bit[5:0] == aer ar AB AM APM aap */
    outl(val, dev->bar_io + PCI_RTL8139_RCR_OFFSET);

    /* 4.set the TCR(transmit config register) */
    val  = (0x3 & 0x3) << 24;           /* IFG:bit[25:24]  == 2b'11 */
    val |= (0x6 & 0x7) << 8;            /* MXDMA:bit[10:8] == 3b'110 */
    val |= (0x8 & 0xF) << 4;            /* TXRR:bit[7:4]   == 4b'1000 */
    outl(val, dev->bar_io + PCI_RTL8139_TCR_OFFSET);

    /* cfg93c46 lock */
    outb(Cfg9346_Lock, dev->bar_io + PCI_RTL8139_CFG93C46_OFFSET);

    /* 5. */
    for (count = 0; count < 4; count++)
    {
        /* set the tx start address register */
        outl((u32)(rtk8139->txDMA) + count * 2048,
             dev->bar_io + PCI_RTL8139_TSAD0 + 4 * count);
    }

	/* 6.make sure RxTx has started */
	val = inb(dev->bar_io + PCI_RTL8139_CR_OFFSET);
	if ((!(val & 0x08)) || (!(val & 0x04)))
		outb(0x0c, dev->bar_io + PCI_RTL8139_CR_OFFSET);


    /* 7.set the IMR(interrupt mask register) */
    val = inw(dev->bar_io + PCI_RTL8139_IMR_OFFSET);
    val |= 0x7FFF;                      /* open all the interrupt except the system err int */
    outw((u16)val, dev->bar_io + PCI_RTL8139_IMR_OFFSET);

    return 0;
}

static vendor_device_t spt_vendev[] = {{PCI_VENDOR_ID_REALTEK, PCI_DEVICE_ID_REALTEK_8139},};

static pci_drv_t realtek8139_drv = {
    .drvname = "realtek8139_drv",
    .vendev = spt_vendev,
    .n_vendev = ARRAY_ELEMENT_SIZE(realtek8139_drv.vendev),
    .pci_init = realtek8139_devinit,
};


/* this is the driver for realtek ethernet chip 10EC:8139 */
static void __init realtek8139init()
{
    /* register the pci drv */
    pcidrv_register(&realtek8139_drv);
}

module_init(realtek8139init, 5);

static void cmd_rtl8139stat_opfunc(char *argv[], int argc, void *param)
{
    u32 count;
    if (argc == 1)
    {
        for (count = 0; count < ARRAY_ELEMENT_SIZE(rtl8139_intstat); count++)
        {
            if ((count % 4) == 0)
                printf("\n");
        
            printf("%2d-->%4d,    ", count + 1, rtl8139_intstat[count]);
        }
        printf("\n");
    }
    else if ((argc == 2) || (argc == 3))
    {
        if (memcmp(argv[1], "rxcatch", sizeof("rxcatch")) == 0)
        {
            if (argc == 3)
            {
                rtl8139_rxdbg = str2num(argv[2]);
            }
            else
            {
                rtl8139_rxdbg = 1;
            }
        }
        else if (memcmp(argv[1], "txcatch", sizeof("txcatch")) == 0)
        {
            if (argc == 3)
            {
                rtl8139_txdbg = str2num(argv[2]);
            }
            else
            {
                rtl8139_txdbg = 1;
            }
        }
        else
        {
            printf("invalid command.\n");
        }
    }
    else
    {
        printf("invalid command.\n");
    }
}

struct command cmd_rtl8139stat _SECTION_(.array.cmd) =
{
    .cmd_name   = "rtl8139stat",
    .info       = "RTL8139 statistic. rxcatch [pkt num], txcatch [pkt num].",
    .op_func    = cmd_rtl8139stat_opfunc,
};


