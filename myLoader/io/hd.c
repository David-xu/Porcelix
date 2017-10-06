#include "typedef.h"
#include "config.h"
#include "section.h"
#include "desc.h"
#include "interrupt.h"
#include "io.h"
#include "hdreg.h"
#include "assert.h"
#include "debug.h"
#include "hd.h"
#include "command.h"
#include "device.h"
#include "block.h"
#include "module.h"
#include "partition.h"

/* we get the harddisk infomation in the core start session. */
struct hd_info hd0_info _SECTION_(.coreentry.param);
struct hd_info hd1_info _SECTION_(.coreentry.param);

/* we get the harddisk partitin table in the core start session. */
struct hd_dptentry hd0_pdt[HD_PDTENTRY_NUM];

static struct hd_request *hd_cur_rq;

static void hd_waitforfree()
{
    u8 status;
    do {
        status = inb(PORT_HD_STAT_COMMAND);
    } while ((status & HD_STATMASK_READY_STAT) == 0);

    DEBUG_HD("hd_waitforfree, status:%#2x\n", status);
}

static inline int wait_DRQ(void)
{
	int retries;
	int stat;

	for (retries = 0; retries < 100000; retries++) {
		stat = inb(PORT_HD_STAT_COMMAND);
		if (stat & HD_STATMASK_DRQ_STAT)
			return 0;
	}
	return -1;
}


void hd_int(struct pt_regs *regs, void *param)
{
    u16 n_hword, *data;

    DEBUG_HD("hd int trace... "
             "hd_cur_rq:0x%#8x PORT_HD_STAT_COMMAND:0x%#2x\n",
             (u32)hd_cur_rq,
             inb(PORT_HD_STAT_COMMAND));

    if (hd_cur_rq == NULL)
        return;


    wait_DRQ();

    data = (u16 *)hd_cur_rq->buff;

    /* if read op, then cp the data into buff */
    if (hd_cur_rq->hdreq == HDREQ_READ)
    {
        n_hword = 256;
        while (n_hword--)
        {
            *data++ = inw(PORT_HD_DATA);
        }
        (hd_cur_rq->sect_num)--;
        hd_cur_rq->buff = data;

        if (hd_cur_rq->sect_num == 0)
        {
            /* send sig */
            hd_cur_rq->flag = 0;
        }
    }
    else if (hd_cur_rq->hdreq == HDREQ_WRITE)
    {
        if (hd_cur_rq->sect_num == 0)
        {
            /* send sig */
            hd_cur_rq->flag = 0;
        }
        else
        {
            wait_DRQ();
            n_hword = 256;
            while (n_hword--)
            {
                outw(*data++, PORT_HD_DATA);
            }
            (hd_cur_rq->sect_num)--;
            hd_cur_rq->buff = data;
        }
    }


}

/*
 * we call this function to transform the
 * partition logicsector idx to the CHS.
 * This functions need to know the harddisc infomation
 * which stored in the 'dev->driver->drv_param'
 * And the partition information
 * which stored in the 'dev->dev_param'
 */
static void hd_partsec2chs(device_t *dev, struct hd_request *rq, u32 part_logicsect)
{
    struct hd_info *hdinfo = (struct hd_info *)(dev->driver->drv_param);
    struct hd_dptentry *partition = (struct hd_dptentry *)(dev->dev_param);
    u32 logicsect = part_logicsect + partition->start_logicsect;
    u32 sectpercyl = ((u32)hdinfo->n_header) * hdinfo->n_sect;

    rq->cyl_start = logicsect / sectpercyl;
    logicsect = logicsect % sectpercyl;
    rq->head_start = logicsect / (hdinfo->n_sect);
    rq->sect_start = 1 + (logicsect % (hdinfo->n_sect));   /* the hd start at: C(0)+H(0)+S(1) */

    DEBUG_HD("     operation : %s\n", rq->hdreq == HDREQ_READ ? "read" : "write");
    DEBUG_HD("      rq->buff : 0x%#8x\n", rq->buff);
    DEBUG_HD("  rq->sect_num : 0x%#8x\n", rq->sect_num);
    DEBUG_HD(" rq->cyl_start : 0x%#8x\n", rq->cyl_start);
    DEBUG_HD("rq->head_start : 0x%#8x\n", rq->head_start);
    DEBUG_HD("rq->sect_start : 0x%#8x\n", rq->sect_start);
}

/*
 * dev  : hd device
 * addr : logic_block index in the partition
 * buff : output buff
 * size : logic_block count
 */
int hd_drv_read(device_t *dev, u32 addr, void *buff, u32 size)
{
    ASSERT(((u32)(buff) % 2) == 0);

    struct hd_request rq = {.hdreq = HDREQ_READ};
    rq.buff = buff;
    rq.sect_num = size * (LOGIC_BLOCK_SIZE / HD_SECTOR_SIZE);
    hd_partsec2chs(dev, &rq, addr * (LOGIC_BLOCK_SIZE / HD_SECTOR_SIZE));

    hd_waitforfree();


    /*  */
    outb(0x08, PORT_HD_CMD),
    /*  */
    outb(0, PORT_HD_ERR_PRECOMP);
    /* sector num */
    outb(rq.sect_num, PORT_HD_NSECTOR);
    /* sector start */
    outb(rq.sect_start, PORT_HD_SECTOR);
    /* cylinder start */
    outb((u8)((rq.cyl_start) >> 8), PORT_HD_HCYL);
    outb((u8)(rq.cyl_start), PORT_HD_LCYL);
    /* header start */
    outb(0xA0 | (rq.head_start), PORT_HD_CURRENT);
    /* launch the read cmd */
    rq.flag = 1;
    hd_cur_rq = &rq;
    barrier();
    outb(HD_CMD_READ, PORT_HD_STAT_COMMAND);
    barrier();

    DEBUG_HD("hd read begin eflag:0x%#8x...\n", native_save_fl());
    while (rq.flag);
    hd_cur_rq = NULL;
    DEBUG_HD("hd read complete.\n");

    return 0;
}

int hd_drv_write(device_t *dev, u32 addr, void *buff, u32 size)
{
    ASSERT(((u32)(buff) % 2) == 0);

    u16 n_hword, *data;
    struct hd_request rq = {.hdreq = HDREQ_WRITE};

    rq.buff = buff;
    rq.sect_num = size * (LOGIC_BLOCK_SIZE / HD_SECTOR_SIZE);
    hd_partsec2chs(dev, &rq, addr * (LOGIC_BLOCK_SIZE / HD_SECTOR_SIZE));
hd_drv_write_retry:

    hd_waitforfree();

    /*  */
    outb(0x08, PORT_HD_CMD),
    /*  */
    outb(0, PORT_HD_ERR_PRECOMP);
    /* sector num */
    outb(rq.sect_num, PORT_HD_NSECTOR);
    /* sector start */
    outb(rq.sect_start, PORT_HD_SECTOR);
    /* cylinder start */
    outb((u8)((rq.cyl_start) >> 8), PORT_HD_HCYL);
    outb((u8)(rq.cyl_start), PORT_HD_LCYL);
    /* header start */
    outb(0xA0 | (rq.head_start), PORT_HD_CURRENT);
    /* launch the write cmd */
    rq.flag = 1;
    hd_cur_rq = &rq;
    barrier();
    outb(HD_CMD_WRITE, PORT_HD_STAT_COMMAND);
    barrier();

    /* wait for the DRQ */
    if (wait_DRQ() != 0)
    {
        goto hd_drv_write_retry;
    }

    /* cp data into the hd hardware buff */
    data = (u16 *)rq.buff;

    n_hword = 256;
    while (n_hword--)
    {
        outw(*data++, PORT_HD_DATA);
    }
    (hd_cur_rq->sect_num)--;
    hd_cur_rq->buff = data;

    barrier();

    DEBUG_HD("hd write begin...\n");
    while (rq.flag);
    hd_cur_rq = NULL;
    DEBUG_HD("hd write complete.\n");

    return 0;
}


static device_driver_t hd0_drv =
{
    .read = hd_drv_read,
    .write = hd_drv_write,

    .drv_param = &hd0_info,
};

static device_driver_t hd1_drv =
{
    .read = hd_drv_read,
    .write = hd_drv_write,

    .drv_param = &hd1_info,
};


/*
 * hd(0/0) hd(0/1) hd(0/2) hd(0/3)
 */
struct partition_desc hdpart_desc[HD_PDTENTRY_NUM] =
{
    {.part_idx = 0},
    {.part_idx = 1},
    {.part_idx = 2},
    {.part_idx = 3},
};

/*
 * this is the cur selected partition.
 */
struct partition_desc *cursel_partition = NULL;


/* this is the harddisk command operation function.
 * -d       : display all the hd partition
 * -m       : mount the hd partition
 */
static void cmd_hdop_opfunc(char *argv[], int argc, void *param)
{
    unsigned i;

    if (argc == 1)
    {
        printk("-d       : display all partion status.\n"
               "-m [part]: mount or display mount partition.\n");
        return;
    }

    /* show all the partition status */
    if (memcmp(argv[1], "-d", strlen("-d")) == 0)
    {
        for (i = 0; i < HD_PDTENTRY_NUM; i++)
        {
            printk("hd(0,%d) info:\n"
                   "bootflag: 0x%#2x, fstype: 0x%#2x\n"
                   "From C:H:S->0x%#3x:0x%#2x:0x%#2x (logic sect NO. 0x%#8x)\n"
                   "To   C:H:S->0x%#3x:0x%#2x:0x%#2x (logic sect NO. 0x%#8x)\n",
                   i, hd0_pdt[i].boot_ind, hd0_pdt[i].sys_ind,
                   hd0_pdt[i].start_cyl, hd0_pdt[i].start_head, hd0_pdt[i].start_sect, hd0_pdt[i].start_logicsect,
                   hd0_pdt[i].end_cyl,   hd0_pdt[i].end_head,   hd0_pdt[i].end_sect,   hd0_pdt[i].start_logicsect + hd0_pdt[i].n_logicsect - 1);
        }
    }
    else if (memcmp(argv[1], "-m", strlen("-m")) == 0)
    {
        /* display the partition mount state */
        if (argc == 2)
        {
            u32 flag = 0;
            struct partition_desc *part_desc;
            for (i = 0; i < HD_PDTENTRY_NUM; i++)
            {
                part_desc = hdpart_desc + i;
                if (part_desc->fs)
                {
                    printk("hd(0,%d): fs type \"%s\".\n", i, part_desc->fs->name);
                    flag = 1;
                }
            }
            if (flag == 0)
            {
                printk("No mounted patition.\n");
            }
        }
        else    /* mount fs */
        {
            unsigned dstpartidx = str2num(argv[2]);
            if (dstpartidx > 3)
            {
                printk("Invalid partition index %d.\n", dstpartidx);
                return;
            }
            struct partition_desc *dstpart = &(hdpart_desc[dstpartidx]);

            /* if the partition has been mounted, we need to umount first. */
            if (dstpart->fs != NULL)
            {
                dstpart->fs->fs_unmount(dstpart->fs, dstpart);
            }

            /* try to mount the dest partion */
            for (i = 0; i < n_supportfs; i++)
            {
                struct file_system *fs = &(fsdesc_array[i]);
                if (fs->regflag == 0)
                {
                    continue;
                }
                if ((fs->fs_mount(fs, dstpart)) == 0)
                {
                    dstpart->fs = fs;
                    cursel_partition = hdpart_desc + dstpartidx;
                    printk("HD(0,%d) mounted as \"%s\" fs.\n", dstpartidx, fs->name);
                    return;
                }
            }

            printk("HD(0,%d) mounted failed.\n", dstpartidx);
        }
    }
    else if (memcmp(argv[1], "-u", strlen("-u")) == 0)
    {
        /* todo: */
    }
    /* set the current activate partition. */
    else if (memcmp(argv[1], "-s", strlen("-s")) == 0)
    {
        if (argc != 3)
        {
            printk("Invalid parameter number.\n");
            return;
        }
        unsigned dstpartidx = str2num(argv[2]);
        if (dstpartidx > 3)
        {
            printk("Invalid partition index %d.\n", dstpartidx);
            return;
        }
        if (hdpart_desc[dstpartidx].fs == NULL)
        {
            printk("HD(0,%d) hasn't been mounted.\n", dstpartidx);
            return;
        }

        cursel_partition = hdpart_desc + dstpartidx;
    }
    /* read one sector */
    else if (memcmp(argv[1], "-r", strlen("-r")) == 0)
    {
        if (argc != 4)
        {
            printk("Invalid parameter number. should be \"-r sector addr.\"\n");
            return;
        }
        u32 sector = str2num(argv[2]);
        u32 addr = str2num(argv[3]);
        device_t *hd0 = gethd_dev(HDPART_HD0_WHOLE);
        hd0->driver->read(hd0, sector, (void *)addr, 1);
        printk("read finished.\n");
    }
    /* write one sector */
    else if (memcmp(argv[1], "-w", strlen("-w")) == 0)
    {
        if (argc != 4)
        {
            printk("Invalid parameter number. should be \"-w sector addr.\"\n");
            return;
        }
        u32 sector = str2num(argv[2]);
        u32 addr = str2num(argv[3]);
        device_t *hd0 = gethd_dev(HDPART_HD0_WHOLE);
        hd0->driver->write(hd0, sector, (void *)addr, 1);
        printk("write finished.\n");
    }

    else
    {
        printk("Unknown option \"%s\".\n", argv[1]);
    }
}

struct command cmd_hdop _SECTION_(.array.cmd) =
{
    .cmd_name   = "hdop",
    .info       = "Harddisk operation. -m [partition], -s [partition], -d, -r [sect] [addr], -w [sect] [addr].",
    .op_func    = cmd_hdop_opfunc,
};

/* total hd */
static struct hd_dptentry logictotalhd = {
    .boot_ind = 0,
    .start_head = 0,
    .start_sect = 0,
    .start_cyl = 0,
    .sys_ind = 0,
    .end_head = 0,
    .end_sect = 0,
    .end_cyl = 0,
    .start_logicsect = 0,
    .n_logicsect = 0,
};

/* whole hd partition */
static device_t hd0_wholehd_dev = {
    .name = "HD(0)", .device_type = DEVTYPE_BLOCK, .driver = &hd0_drv, .dev_param = &logictotalhd,
};

/* whole hd partition */
static device_t hd1_wholehd_dev = {
    .name = "HD(1)", .device_type = DEVTYPE_BLOCK, .driver = &hd1_drv, .dev_param = &logictotalhd,
};


/* hd logic partition */
static device_t hd0_logicpart_devlist[HD_PDTENTRY_NUM] = {
    {.name = "HD(0,0)", .device_type = DEVTYPE_BLOCK, .driver = &hd0_drv, .dev_param = &(hd0_pdt[0])},
    {.name = "HD(0,1)", .device_type = DEVTYPE_BLOCK, .driver = &hd0_drv, .dev_param = &(hd0_pdt[1])},
    {.name = "HD(0,2)", .device_type = DEVTYPE_BLOCK, .driver = &hd0_drv, .dev_param = &(hd0_pdt[2])},
    {.name = "HD(0,3)", .device_type = DEVTYPE_BLOCK, .driver = &hd0_drv, .dev_param = &(hd0_pdt[3])}
};

device_t *gethd_dev(u8 part)
{
    if (part == HDPART_HD0_WHOLE)
    {
        return &hd0_wholehd_dev;
    }
    else if (part == HDPART_HD1_WHOLE)
    {
        return &hd1_wholehd_dev;
    }

    return &(hd0_logicpart_devlist[part]);
}

static void hd_readpdt(void)
{
#if 0
    for (i = 0; i < HD_PDTENTRY_NUM; i++)
    {
        hd0_pdt[i] = bootparam.hd_pdt[i];
    }
#else
	/* we read the hd partition table from HD directly */
	device_t *hd0 = gethd_dev(HDPART_HD0_WHOLE);
	void *tmppage = page_alloc(BUDDY_RANK_4K, MMAREA_NORMAL);
	hd0->driver->read(hd0, 0, tmppage, 1);
	memcpy(hd0_pdt,
		   tmppage + 512 - 2 - 4 * sizeof(struct hd_dptentry),
		   4 * sizeof(struct hd_dptentry));
	page_free(tmppage);
#endif
}

static void __init hd_init(void)
{
    hd_cur_rq = NULL;

	/* harddisk interrupt */
	interrup_register(X86_VECTOR_IRQ_2E - X86_VECTOR_IRQ_20,
					  hd_int, NULL);
    u32 i;
    for (i = 0; i < HD_PDTENTRY_NUM; i++)
    {
        hdpart_desc[i].dev = hd0_logicpart_devlist + i;
    }

	hd_readpdt();
}

// module_init(hd_init, 2);


