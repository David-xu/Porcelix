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

/* we get the harddisk infomation in the core start session. */
struct hd_info hd0_info /* _SECTION_(.init.data) */;

/* we get the harddisk partitin table in the core start session. */
struct hd_dptentry hd0_pdt[HD_PDTENTRY_NUM] /* _SECTION_(.init.data) */;

static struct hd_request *hd_cur_rq;

void hd_int(void)
{
    u8 n_sec;
    u16 n_hword, *data;

    DEBUG("hd int trace...\n");

    if (hd_cur_rq == NULL)
        return;

    n_sec = hd_cur_rq->sect_num;
    data = (u16 *)hd_cur_rq->buff;

    /* if read op, then cp the data into buff */
    if (hd_cur_rq->hdreq == HDREQ_READ)
    {
        while (n_sec--)
        {
            n_hword = 256;
            while (n_hword--)
            {
                *data++ = inw(PORT_HD_DATA);
            }
        }
    }

    /* send sig */
    hd_cur_rq->flag = 0;
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

    DEBUG("     operation : %s\n", rq->hdreq == HDREQ_READ ? "read" : "write");
    DEBUG("      rq->buff : 0x%#8x\n", rq->buff);
    DEBUG("  rq->sect_num : 0x%#8x\n", rq->sect_num);
    DEBUG(" rq->cyl_start : 0x%#8x\n", rq->cyl_start);
    DEBUG("rq->head_start : 0x%#8x\n", rq->head_start);
    DEBUG("rq->sect_start : 0x%#8x\n", rq->sect_start);
}

static void hd_waitforfree()
{
    u8 status;
    do {
        status = inb(PORT_HD_STAT_COMMAND);
    } while ((status & HD_STATMASK_READY_STAT) == 0);

    DEBUG("hd_waitforfree, status:%#2x\n", status);
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
    outb(HD_CMD_READ, PORT_HD_STAT_COMMAND);

    DEBUG("hd read begin...\n");
    while (rq.flag);
    hd_cur_rq = NULL;
    DEBUG("hd read complete.\n");

    return 0;
}

int hd_drv_write(device_t *dev, u32 addr, void *buff, u32 size)
{
    ASSERT(((u32)(buff) % 2) == 0);

    u32 n_sec;
    u16 n_hword, *data;
    struct hd_request rq = {.hdreq = HDREQ_WRITE};

    rq.buff = buff;
    rq.sect_num = size * (LOGIC_BLOCK_SIZE / HD_SECTOR_SIZE);
    hd_partsec2chs(dev, &rq, addr * (LOGIC_BLOCK_SIZE / HD_SECTOR_SIZE));

    hd_waitforfree();
    
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
    outb(HD_CMD_WRITE, PORT_HD_STAT_COMMAND);

    /* cp data into the hd hardware buff */
    n_sec = rq.sect_num;
    data = (u16 *)rq.buff;

    while (n_sec--)
    {
        n_hword = 256;
        while (n_hword--)
        {
            outw(*data++, PORT_HD_DATA);
        }
    }

    DEBUG("hd write begin...\n");
    while (rq.flag);
    hd_cur_rq = NULL;
    DEBUG("hd write complete.\n");

    return 0;
}


static device_driver_t hd_drv =
{
    .read = hd_drv_read,
    .write = hd_drv_write,

    .drv_param = &hd0_info,
};

void hd_init(void)
{
    u8 intmask;

    hd_cur_rq = NULL;

    _cli();

    /* OCW1, set 8259A slaver INTMASK reg */
    intmask = inb(PORT_8259A_SLAVER_A1);    /* the INTMAST reg is the 0x21 and 0xA1 */
    outb(intmask & (~0x40), PORT_8259A_SLAVER_A1);
    
    set_idt(X86_VECTOR_IRQ_2E, hd_int_entrance);        /* harddisk interrupt */

    _sti();

    static device_t hd_devlist[HD_PDTENTRY_NUM] = {
        {.name = "HD(0,0)", .device_type = DEVTYPE_BLOCK, .driver = &hd_drv, .dev_param = &(hd0_pdt[0])},
        {.name = "HD(0,1)", .device_type = DEVTYPE_BLOCK, .driver = &hd_drv, .dev_param = &(hd0_pdt[1])},
        {.name = "HD(0,2)", .device_type = DEVTYPE_BLOCK, .driver = &hd_drv, .dev_param = &(hd0_pdt[2])},
        {.name = "HD(0,3)", .device_type = DEVTYPE_BLOCK, .driver = &hd_drv, .dev_param = &(hd0_pdt[3])}};
    u32 i;
    for (i = 0; i < HD_PDTENTRY_NUM; i++)
    {
        hdpart_desc[i].dev = hd_devlist + i;
    }
}


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
static void cmd_hdop_opfunc(u8 *argv[], u8 argc, void *param)
{
    unsigned i;

    if (argc == 1)
    {
        printf("-d       : display all partion status.\n"
               "-m [part]: mount or display mount partition.\n");
        return;
    }
    
    /* show all the partition status */
    if (memcmp(argv[1], "-d", strlen("-d")) == 0)
    {
        for (i = 0; i < HD_PDTENTRY_NUM; i++)
        {
            printf("hd(0,%d) info:\n"
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
                    printf("hd(0,%d): fs type \"%s\".\n", i, part_desc->fs->name);
                    flag = 1;
                }
            }
            if (flag == 0)
            {
                printf("No mounted patition.\n");
            }
        }
        else    /* mount fs */
        {
            unsigned dstpartidx = str2num(argv[2]);
            if (dstpartidx > 3)
            {
                printf("Invalid partition index %d.\n", dstpartidx);
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
                    printf("HD(0,%d) mounted as \"%s\" fs.\n", dstpartidx, fs->name);
                    return;
                }
            }
            
            printf("HD(0,%d) mounted failed.\n", dstpartidx);
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
            printf("Invalid parameter number.\n");
            return;
        }
        unsigned dstpartidx = str2num(argv[2]);
        if (dstpartidx > 3)
        {
            printf("Invalid partition index %d.\n", dstpartidx);
            return;
        }
        if (hdpart_desc[dstpartidx].fs == NULL)
        {
            printf("HD(0,%d) hasn't been mounted.\n", dstpartidx);
            return;
        }
        
        cursel_partition = hdpart_desc + dstpartidx;
    }
    else
    {
        printf("Unknown option \"%s\".\n", argv[1]);
    }
}

struct command cmd_hdop _SECTION_(.array.cmd) =
{
    .cmd_name   = "hdop",
    .info       = "Harddisk operation. -m [partition], -s [partition], -d.",
    .op_func    = cmd_hdop_opfunc,
};


