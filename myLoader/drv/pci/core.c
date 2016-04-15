#include "config.h"
#include "typedef.h"
#include "public.h"
#include "pci.h"
#include "io.h"
#include "command.h"
#include "section.h"
#include "ml_string.h"
#include "list.h"
#include "memory.h"
#include "module.h"

static LIST_HEAD(pcidevlist);
static LIST_HEAD(pcidrvlist);

static void cmd_dispci_opfunc(char *argv[], int argc, void *param)
{
    u32 bus, dev, func, count;
    pcicfgdata_t pcicfg;

    if ((argc == 1) ||
        ((argc == 2) && (memcmp("-a", argv[1], sizeof("-a")) == 0)))
    {
        printk("PCI bus enumeration begin...\n");
        
        for (bus = 0; bus < PCI_BUS_COUNT; bus++)
            for (dev = 0; dev < PCI_DEV_COUNT; dev++)
                for (func = 0; func < PCI_FUNC_COUNT; func++)
                {
                    getpciinfo(bus, dev, func, &pcicfg);
					if ((pcicfg.u.cfg.vendor == 0x15ad) &&
						(pcicfg.u.cfg.device == 0x7a0))
						continue;
                    
                    if (pcicfg.u.cfg.vendor != PCI_INVALID_VENDORID)
                    {
                        printk("bus:%2d dev:%2x func:%x ---> Vendor ID:%4x, Device ID:%4x.\n",
                               bus, dev, func,
                               pcicfg.u.cfg.vendor, pcicfg.u.cfg.device);
                    }
                }
    }
    else if ((argc == 3) &&
             (memcmp("-b", argv[1], sizeof("-b")) == 0))
    {
        bus = str2num(argv[2]);
        count = 0;
        for (dev = 0; dev < PCI_DEV_COUNT; dev++)
            for (func = 0; func < PCI_FUNC_COUNT; func++)
            {
                getpciinfo(bus, dev, func, &pcicfg);
            
                if (pcicfg.u.cfg.vendor != PCI_INVALID_VENDORID)
                {
                    printk("bus:%2d dev:%2x func:%x ---> Vendor ID:%4x, Device ID:%4x.\n",
                           bus, dev, func,
                           pcicfg.u.cfg.vendor, pcicfg.u.cfg.device);
                    count++;
                }
            }
        if (count == 0)
        {
            printk("No PCI devices on bus:%d.\n", bus);
        }
    }
    else if ((argc == 5) &&
             (memcmp("-bdf", argv[1], sizeof("-bdf")) == 0))
    {
        bus = str2num(argv[2]);
        dev = str2num(argv[3]);
        func = str2num(argv[4]);

        getpciinfo(bus, dev, func, &pcicfg);
        
        if ((pcicfg.u.cfg.vendor) == PCI_INVALID_VENDORID)
        {
            printk("Device (%d) with func (%d) on bus (%d) dosn't exist.\n", dev, func, bus);
            return;
        }

        printk("Device (%d) with func (%d) on bus(%d):\n", dev, func, bus);
        pci_dispcfgdata(&pcicfg);
    }
}

struct command cmd_dispcidev _SECTION_(.array.cmd) =
{
    .cmd_name   = "dispci",
    .info       = "Display all PCI device in the system. -a, -b [bus], -bdf [bus] [dev] [func].",
    .op_func    = cmd_dispci_opfunc,
};



void pci_dispcfgdata(pcicfgdata_t *pcicfg)
{
    unsigned i = 0;
    printk("   Vendor: 0x%#4x,  Device: 0x%#4x,     SubVer: 0x%#4x,  SubDev: 0x%#4x\n"
           "    class: 0x%#2x,  subclass: 0x%#2x,         prog: 0x%#2x, sRevision: 0x%#2x\n"
           "     BIST: 0x%#2x,HeaderType: 0x%#2x, LatencyTimer: 0x%#2x, CacheLineSize: 0x%#2x\n"
           "   MaxLat: 0x%#2x,    MinGnt: 0x%#2x,       IntPin: 0x%#2x,       IntLine: 0x%#2x\n",
           pcicfg->u.cfg.vendor, pcicfg->u.cfg.device, pcicfg->u.cfg.subvendor, pcicfg->u.cfg.subid,
           pcicfg->u.cfg.class, pcicfg->u.cfg.subclass, pcicfg->u.cfg.prog, pcicfg->u.cfg.revision,
           pcicfg->u.cfg.bist, pcicfg->u.cfg.ht, pcicfg->u.cfg.lt, pcicfg->u.cfg.cls,
           pcicfg->u.cfg.maxlat, pcicfg->u.cfg.mingnt, pcicfg->u.cfg.intpin, pcicfg->u.cfg.intline);
    for (i = 0; i < ARRAY_ELEMENT_SIZE(pcicfg->u.cfg.baseaddr); i++)
    {
        printk("BAR(%d):0x%#8x    BAR(%d)mask:0x%#8x\n",
               i, pcicfg->u.cfg.baseaddr[i], i, pcicfg->bar_mask[i]);
    }
}

int pcidrv_register(pci_drv_t *drv)
{
    pci_drv_t *p;
    /**/
    LIST_FOREACH_ELEMENT(p, &pcidrvlist, drvlist)
    {
        if (memcmp(drv->drvname, p->drvname,strlen(drv->drvname)) == 0)
        {
            return -1;
        }
    }

    list_add_head(&(drv->drvlist), &pcidrvlist);
    return 0;
}

int pcidev_register(pci_dev_t *pcidev)
{
    pci_dev_t *p;
    LIST_FOREACH_ELEMENT(p, &pcidevlist, devlist)
    {
        if ((p->pcicfg.u.cfg.vendor == pcidev->pcicfg.u.cfg.vendor) &&
            (p->pcicfg.u.cfg.device == pcidev->pcicfg.u.cfg.device))
        {
            return -1;
        }
    }

    list_add_head(&(pcidev->devlist), &pcidevlist);
    return 0;
}

void getpciinfo(u32 bus, u32 dev, u32 func, pcicfgdata_t *pcicfg)
{
    u32 count, cfgword, val;
    
    cfgword = PCI_MAKE_CFGWORD(bus, dev, func);
    outl(cfgword, PCI_CONFIG_ADDRESS);
    val = inl(PCI_DATA_ADDRESS);
    if (((u16)val) == PCI_INVALID_VENDORID)
    {
        pcicfg->u.cfg.vendor = PCI_INVALID_VENDORID;
        return;
    }

    /**/
    pcicfg->bus = bus;
    pcicfg->dev = dev;
    pcicfg->func = func;

    for (count = 0; count < ARRAY_ELEMENT_SIZE(pcicfg->u.word); count++)
    {
        outl(cfgword, PCI_CONFIG_ADDRESS);
        pcicfg->u.word[count] = inl(PCI_DATA_ADDRESS);
        cfgword += sizeof(u32);
    }

    /* get barinfo */
    cfgword = PCI_MAKE_CFGWORD(bus, dev, func);
    cfgword += PCI_CFG_BAR0_OFFSET;

    /* write the 0xffffffff to bar and read, after that we can get the bar_mask */
    for (count = 0; count < ARRAY_ELEMENT_SIZE(pcicfg->u.cfg.baseaddr); count++)
    {
    	/* write 0xffffffff to bar */
        outl(cfgword, PCI_CONFIG_ADDRESS);
        outl(0xFFFFFFFF, PCI_DATA_ADDRESS);

		/* read it, and we can get the mask */
        outl(cfgword, PCI_CONFIG_ADDRESS);
        val = inl(PCI_DATA_ADDRESS);
        pcicfg->bar_mask[count] = val & (~0xF);

		/* resume the bar value */
        outl(cfgword, PCI_CONFIG_ADDRESS);
        outl(pcicfg->u.cfg.baseaddr[count], PCI_DATA_ADDRESS);

        cfgword += 4;
    }
}

void pci_init(void)
{
    /* enum all the PCI devices */
    u32 bus, dev, func, count;
    pcicfgdata_t pcicfg;
    pci_drv_t *pcidrv;
    pci_dev_t *pcinewdev;

    /* enum all the pci device, find fit driver for each */
    for (bus = 0; bus < PCI_BUS_COUNT; bus++)
        for (dev = 0; dev < PCI_DEV_COUNT; dev++)
            for (func = 0; func < PCI_FUNC_COUNT; func++)
            {
                getpciinfo(bus, dev, func, &pcicfg);
                if (pcicfg.u.cfg.vendor == PCI_INVALID_VENDORID)
                {
                    continue;
                }

                /* new device, cause the space after pci_dev_t in this page, we used it
                 * so we need to page_alloc
				 */
                pcinewdev = (pci_dev_t *)page_alloc(BUDDY_RANK_4K, MMAREA_LOW1M);
                
                /* copy the pci config data into the device struct */
                memcpy(&(pcinewdev->pcicfg), &pcicfg, sizeof(pcinewdev->pcicfg));
                /* copy the bus dev func */

                /* try to find one proper driver */                
                LIST_FOREACH_ELEMENT(pcidrv, &pcidrvlist, drvlist)
                {
                    for (count = 0; count < pcidrv->n_vendev; count++)
                    {
                        /* get one fit driver */
                        if ((pcidrv->vendev[count].vendor == pcicfg.u.cfg.vendor) &&
                            (pcidrv->vendev[count].device == pcicfg.u.cfg.device))
                        {
                            /* try to do the driver init */
                            if ((pcidrv->pci_init(pcinewdev)) != 0)
                            {
                                /* driver init failed, try next */
                                continue;
                            }

                            /* ok we have find one proper driver for this PCI dev */
                            goto findoneproperdrv;
                        }
                    }
                }
                
                /* release the page */
                page_free(pcinewdev);
                continue;

findoneproperdrv:
                pcinewdev->pcidrv = pcidrv;
                if (pcidev_register(pcinewdev) == 0)
                {
                    printk("pci device %#4x:%#4x register success, driver name:%s\n",
                           pcinewdev->pcicfg.u.cfg.vendor, pcinewdev->pcicfg.u.cfg.device,
                           pcinewdev->pcidrv->drvname);
                }
                else
                {

                }
            }
}

module_init(pci_init, 6);


