#include "public.h"
#include "pci.h"
#include "module.h"
#include "interrupt.h"
#include "net.h"
#include "io.h"
#include "memory.h"
#include "ml_string.h"
#include "debug.h"
#include "command.h"

/* some pci vendor device */
#define PCI_VENDOR_ID_INTEL			0x8086
#define PCI_DEVICE_ID_INTEL_E1000	0x100E

static const u32 e1000_macaddr[2] = {0x5254, 0x00123456};

/* INT FLAG */
#define	E1000INTFLAG_RXT0			(0x00000080)

/* register list */
#define E1000REG_CTRL				(0x0000)
#define E1000REG_ICR				(0x00C0)
#define E1000REG_IMR				(0x00D0)
#define E1000REG_IMC				(0x00D8)

/* receive control register */
#define	E1000REG_RCTL				(0x100)

/* transmit control register */
#define	E1000REG_TCTL				(0x400)

/* some receive descriptor registers */
#define E1000REG_RDBAL				(0x2800)
#define E1000REG_RDBAH				(0x2804)
#define E1000REG_RDLEN				(0x2808)
#define E1000REG_RDH				(0x2810)
#define E1000REG_RDT				(0x2818)

/* some transmit descriptor registers */
#define E1000REG_TDBAL				(0x3800)
#define E1000REG_TDBAH				(0x3804)
#define E1000REG_TDLEN				(0x3808)
#define	E1000REG_TDH				(0x3810)
#define	E1000REG_TDT				(0x3818)

#define E1000REG_N_RA				(16)
#define	E1000REG_RAL_BASS			(0x5400)
#define	E1000REG_RAH_BASS			(0x5404)
#define	E1000REG_RAL(n)				(E1000REG_RAL_BASS + ((n) << 3))
#define	E1000REG_RAH(n)				(E1000REG_RAH_BASS + ((n) << 3))

typedef struct e1000_rxdesc {
	u64		buffaddr;
 	u16		length;		// Length of data DMAed into data buffer
	u16		csum;		// Packet checksum
	u8		status;		// Descriptor status
	u8		errors;		// Descriptor Errors
	u16		special;
} e1000_rxdesc_t;

typedef struct e1000_txdesc {
	u64		buffaddr;
	union {
		u32		data;
		struct {
			u16		length;		// Data buffer length
			u8		cso;		// Checksum offset
			u8		cmd;		// Descriptor control
		} flags;
	} lower;
	union {
		u32		data;
		struct {
			u8		status;		// Descriptor status
			u8		css;		// Checksum start
			u16		special;
		} fields;
	} upper;
} e1000_txdesc_t;

/* E1000_REGIOPORT
 * 'IOADDR' register: io_bar + 0
 * 'IODATA' register: io_bar + 4
 * config the 'IOADDR' with dest register offset value (param 'off')
 * read (write) the value from(into) 'IODATA'
 */
#define E1000_REGIOPORT				(0)
static void e1000_writeR(struct pci_dev *dev, u32 off, u32 value)
{
#if	E1000_REGIOPORT==1
	outl(off, dev->bar_io);
	outl(value, dev->bar_io + 4);
#else
	*((volatile u32 *)(dev->bar_mem + off)) = value;
#endif
}
static u32 e1000_readR(struct pci_dev *dev, u32 off)
{
	volatile u32 ret;
#if	E1000_REGIOPORT==1
	outl(off, dev->bar_io);
	ret = inl(dev->bar_io + 4);
#else
	ret = *((u32 *)(dev->bar_mem + off));
#endif
	return ret;
}

static u32 e1000_intstat[16] = {0};

static void testsub_dispcpu()
{
	u32 cur_cs, cur_ds, cur_es, cur_ss, cur_esp;
	__asm__ __volatile__ (
		"movl	%%cs, %%eax			\n\t"
		"movl	%%eax, %0			\n\t"

		"movl	%%ds, %%eax			\n\t"
		"movl	%%eax, %1			\n\t"
		"movl	%%es, %%eax			\n\t"
		"movl	%%eax, %2			\n\t"

		"movl	%%ss, %%eax			\n\t"
		"movl	%%eax, %3			\n\t"

		"movl	%%esp, %4			\n\t"
		:"=m"(cur_cs), "=m"(cur_ds), "=m"(cur_es), 
		 "=m"(cur_ss), "=m"(cur_esp)
		:
		:"%eax"
	);

	printk("current regs:\n"
		   "cs: 0x%#4x, ds: 0x%#4x, es: 0x%#4x, ss: 0x%#4x\n"
		   "esp: 0x%#8x\n"
		   "CR0: 0x%#8x, CR3: 0x%#8x\n",
		   cur_cs, cur_ds, cur_es, cur_ss,
		   cur_esp,
		   getCR0(), getCR3());
}

static void intele1000_isr(struct pt_regs *regs, void *param)
{
	testsub_dispcpu();

	struct pci_dev *dev = (struct pci_dev *)param;
	netdev_t *e1000 = (netdev_t *)dev->param;
	e1000_rxdesc_t *rdbuff = (e1000_rxdesc_t *)(e1000->rxdexc_ring);
	u32 icr = e1000_readR(dev, E1000REG_ICR);
{
    u32 count, flag = 0x1;
    for (count = 0; count < 16; count++)
    {
        if (flag & icr)
        {
            e1000_intstat[count]++;
        }
        flag <<= 1;
    }
}

	// printk("icr:0x%#8x.\n", icr);
	if (E1000INTFLAG_RXT0 & icr)
	{
		u32 rdh = e1000_readR(dev, E1000REG_RDH);
		u32	rdt = e1000_readR(dev, E1000REG_RDT);
		while (rdt != rdh)
		{
#if 1
			/* receive a packet and insert in to the eh_proc */
	        ethframe_t *new_ef;

            new_ef = (ethframe_t *)page_alloc(BUDDY_RANK_4K, MMAREA_NORMAL);
            new_ef->netdev = e1000;
            new_ef->len = rdbuff[rdt].length;
            new_ef->buf = new_ef->bufs + 128;
            memcpy(new_ef->buf, (void *)((u32)(rdbuff[rdt].buffaddr)), new_ef->len);

            rxef_insert(new_ef);
#else
			dump_ram((void *)(u32)(rdbuff[rdt].buffaddr), rdbuff[rdt].length);
#endif
			rdt = (rdt + 1) % (e1000->n_rxdesc);
		}

		e1000_writeR(dev, E1000REG_RDT, rdt);
	}
}

static int intele1000_tx(netdev_t *netdev, void *buf, u32 len)
{
	struct pci_dev *dev = netdev->dev;
	e1000_txdesc_t *txbuff = (e1000_txdesc_t *)(netdev->txdexc_ring);

	u32 tdh = e1000_readR(dev, E1000REG_TDH);
	u32	tdt_op, tdt = e1000_readR(dev, E1000REG_TDT);

	tdt_op = tdt;
	tdt = (tdt + 1) % (netdev->n_txdesc);
	if (tdt == tdh)
	{
		printk("intele1000_tx() faild.\n");
		/* no enough transmit descriptor */
		return -1;
	}

	memcpy((void *)(u32)(txbuff[tdt_op].buffaddr), buf, len);
	txbuff[tdt_op].lower.flags.length = (u16)len;
	txbuff[tdt_op].lower.flags.cmd = 3;

	e1000_writeR(dev, E1000REG_TDT, tdt);

	return 0;
}

/* init receive descriptor */
static void intele1000_initrxdsc(netdev_t *e1000, u32 descbuff, u16 n_desc)
{
	u32 count;
	u32 rbbase = (u32)(e1000->rxDMA);
	ASSERT(sizeof(e1000_rxdesc_t) == 16);
	e1000_rxdesc_t *rd = (e1000_rxdesc_t *)descbuff;
	for (count = 0; count < n_desc; count++)
	{
		rd->buffaddr = rbbase;

		rbbase += e1000->rd_bufsize;
		rd++;
	}
}

/* init transmit descriptor */
static void intele1000_inittxdsc(netdev_t *e1000, u32 descbuff, u16 n_desc)
{
	u32 count;
	u32 rbbase = (u32)(e1000->txDMA);
	ASSERT(sizeof(e1000_txdesc_t) == 16);
	e1000_txdesc_t *td = (e1000_txdesc_t *)descbuff;
	for (count = 0; count < n_desc; count++)
	{
		td->buffaddr = rbbase;

		rbbase += e1000->td_bufsize;
		td++;
	}
}

/*
 * | 4K align  |           | padding ...    | 512B align
 * pci_dev     netdev_t                     receive descriptor buff
 */
static int intele1000_devinit(struct pci_dev *dev)
{
    u32 count;
    netdev_t *e1000 = (netdev_t *)(dev + 1);
	u32 rdbuff = ((u32)e1000 + 511) & (~0x1FF);
	u32 tdbuff = rdbuff + 256;
    
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
            dev->bar_io = (u16)(dev->pcicfg.u.cfg.baseaddr[count] &
								dev->pcicfg.bar_mask[count]);
        }
        /* MEM BAR */
        else
        {
#if 0
            dev->bar_mem = dev->pcicfg.u.cfg.baseaddr[count] &
						   dev->pcicfg.bar_mask[count];
			// printk("dev->bar_mem : 0x%#8x\n", dev->bar_mem);

#else
        	u32 offset, range = (~(dev->pcicfg.bar_mask[count])) + 1 + PAGE_SIZE;
			u32 va, pa;
            dev->bar_mem = dev->pcicfg.u.cfg.baseaddr[count] &
						   dev->pcicfg.bar_mask[count];
			va = 0x10000000 + 0x1000;
			pa = dev->bar_mem & (~(PAGE_SIZE - 1));

			offset = dev->bar_mem - pa;
			/* let's remap this addr */
			mmap(va, pa, range, 0);

			dev->bar_mem = va + offset;
#endif
        }
    }

	e1000->n_rxdesc = 16;		/* totally 16 receive descriptor */
	e1000->n_txdesc = 16;
	e1000->rd_bufsize = 2048;
	e1000->td_bufsize = 2048;
	e1000->rxdexc_ring = (void *)rdbuff;
	e1000->txdexc_ring = (void *)tdbuff;
	e1000->rxDMA = page_alloc(get_pagerank(e1000->rd_bufsize * e1000->n_rxdesc), MMAREA_LOW1M);
	e1000->txDMA = page_alloc(get_pagerank(e1000->td_bufsize * e1000->n_txdesc), MMAREA_LOW1M);
	intele1000_initrxdsc(e1000, rdbuff, e1000->n_rxdesc);
	intele1000_inittxdsc(e1000, tdbuff, e1000->n_txdesc);
	e1000->tx = intele1000_tx;
	e1000->dev = dev;
	dev->param = e1000;

	/* init the int mask */
	e1000_writeR(dev, E1000REG_IMR, 0xDC);

    /* register the int */
    if (interrup_register(dev->pcicfg.u.cfg.intline, intele1000_isr, dev) != 0)
    {
        printk("intele1000_isr register failed.\n");
        return -1;
    }

	/* MAC addr register */
	e1000->macaddr[0] = (u8)(e1000_macaddr[0] >> 8);
	e1000->macaddr[1] = (u8)(e1000_macaddr[0] >> 0);
	e1000->macaddr[2] = (u8)(e1000_macaddr[1] >> 24);
	e1000->macaddr[3] = (u8)(e1000_macaddr[1] >> 16);
	e1000->macaddr[4] = (u8)(e1000_macaddr[1] >> 8);
	e1000->macaddr[5] = (u8)(e1000_macaddr[1] >> 0);
	// e1000_writeR(dev, E1000REG_RAH(0), e1000_macaddr[0] | 0x80000000);
	// e1000_writeR(dev, E1000REG_RAL(0), e1000_macaddr[1]);
	e1000_writeR(dev, E1000REG_RAH(0), 0x5634 | 0x80000000);
	e1000_writeR(dev, E1000REG_RAL(0), 0x12005452);

	/* receive descriptor config */
	e1000_writeR(dev, E1000REG_RDBAL, rdbuff);		/* the rd buff should align to 16 byte */
	e1000_writeR(dev, E1000REG_RDBAH, 0);
	e1000_writeR(dev, E1000REG_RDLEN, 256);			/* the rd buff length should align to 128 byte */	
	e1000_writeR(dev, E1000REG_RDH, 0);				/* RD header */
	e1000_writeR(dev, E1000REG_RDT, 0);				/* RD tailer */

	/* receive control register, start to receive */
	e1000_writeR(dev, E1000REG_RCTL, 0x4008202);

	/* transmit descriptor config */
	e1000_writeR(dev, E1000REG_TDBAL, tdbuff);		/* the td buff should align to 16 byte */
	e1000_writeR(dev, E1000REG_TDBAH, 0);
	e1000_writeR(dev, E1000REG_TDLEN, 256);			/* the td buff length should align to 128 byte */	
	e1000_writeR(dev, E1000REG_TDH, 0);				/* TD header */
	e1000_writeR(dev, E1000REG_TDT, 0);				/* TD tailer */
	
	/* transmit control register, start to transmit */
	e1000_writeR(dev, E1000REG_TCTL, 0x4010a);
	
	return 0;
}


static vendor_device_t spt_vendev[] = {{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_E1000},};

static pci_drv_t intelw1000_drv = {
    .drvname = "intelw1000_drv",
    .vendev = spt_vendev,
    .n_vendev = ARRAY_ELEMENT_SIZE(intelw1000_drv.vendev),
    .pci_init = intele1000_devinit,
};


/* this is the driver for realtek ethernet chip 10EC:8139 */
static void __init intele1000init()
{
    /* register the pci drv */
    pcidrv_register(&intelw1000_drv);
}

module_init(intele1000init, 5);

static void cmd_e1000stat_opfunc(char *argv[], int argc, void *param)
{
    u32 count;
    if (argc == 1)
    {
        for (count = 0; count < ARRAY_ELEMENT_SIZE(e1000_intstat); count++)
        {
            if ((count % 4) == 0)
                printk("\n");
        
            printk("%2d-->%4d,    ", count + 1, e1000_intstat[count]);
        }
        printk("\n");
    }
    else
    {
        printk("invalid command.\n");
    }
}

struct command cmd_e1000stat _SECTION_(.array.cmd) =
{
    .cmd_name   = "e1000stat",
    .info       = "e1000 statistic.",
    .op_func    = cmd_e1000stat_opfunc,
};

