#ifndef _PCI_H_
#define _PCI_H_

#include "public.h"
#include "list.h"

#define PCI_CONFIG_ADDRESS              (0xCF8)
#define PCI_DATA_ADDRESS                (0xCFC)

#define PCI_REGIDX_OFFSET               (0)
#define PCI_REGIDX_WIDTH                (8)

#define PCI_FUNC_OFFSET                 (PCI_REGIDX_OFFSET + PCI_REGIDX_WIDTH)
#define PCI_FUNC_WIDTH                  (3)
#define PCI_FUNC_COUNT                  (0x1 << PCI_FUNC_WIDTH)

#define PCI_DEV_OFFSET                  (PCI_FUNC_OFFSET + PCI_FUNC_WIDTH)
#define PCI_DEV_WIDTH                   (5)
#define PCI_DEV_COUNT                   (0x1 << PCI_DEV_WIDTH)

#define PCI_BUS_OFFSET                  (PCI_DEV_OFFSET + PCI_DEV_WIDTH)
#define PCI_BUS_WIDTH                   (8)
#define PCI_BUS_COUNT                   (0x1 << PCI_BUS_WIDTH)

#define PCI_CFG_COMMAND					(0x4)

#define PCI_CFG_BAR0_OFFSET             (0x10)
#define PCI_CFG_BAR1_OFFSET             (0x14)
#define PCI_CFG_BAR2_OFFSET             (0x18)
#define PCI_CFG_BAR3_OFFSET             (0x20)
#define PCI_CFG_BAR4_OFFSET             (0x24)
#define PCI_CFG_BAR5_OFFSET             (0x28)


#define PCI_INVALID_VENDORID            (0xFFFF)

#define PCI_MAKE_CFGWORD(bus, dev, func)                                \
        (0x80000000) |                                                  \
        (((bus) & PUBLIC_GETMASK(PCI_BUS_WIDTH)) << PCI_BUS_OFFSET) |   \
        (((dev) & PUBLIC_GETMASK(PCI_DEV_WIDTH)) << PCI_DEV_OFFSET) |   \
        (((func) & PUBLIC_GETMASK(PCI_FUNC_WIDTH)) << PCI_FUNC_OFFSET)

typedef struct pcicfgdata {
    union {
        u32 word[16];
        struct pcicfg {
            u32     vendor  : 16;
            u32     device  : 16;

            u32     command : 16;
            u32     status  : 16;

            u32     revision: 8;
			u32     prog	: 8;
			u32     subclass: 8;
            u32     class   : 8;

            u32     cls     : 8;    /* cache line size */
            u32     lt      : 8;    /* latency timer */
            u32     ht      : 8;    /* header type */
            u32     bist    : 8;

            u32     baseaddr[6];

            u32     CCP;            /* cardbus CIS pointer */

            u32     subvendor: 16;  /* subvendor id */
            u32     subid   : 16;   /* subsystem id */

            u32     expROMBASS;     /* Expansion ROMBase Addr */

            u32     CP      : 8;    /* Capabilities Pointer */
            u32     rsv0    : 24;

            u32     rsv1;

            u32     intline : 8;    /* Interrupt Line */
            u32     intpin  : 8;    /* Interrupt Pin */
            u32     mingnt  : 8;    /* min_gnt */
            u32     maxlat  : 8;    /* max_lat */
        } cfg;
    } u;

    u32 bar_mask[6];

    u8 bus, dev, func;
} pcicfgdata_t;

/* device list which this driver supportted */
typedef struct vendor_device{
    u16 vendor;
    u16 device;
} vendor_device_t;


struct pci_dev;

typedef struct pci_drv {
    struct list_head drvlist;
    char *drvname;

    /* device list which this driver supportted */
    vendor_device_t *vendev;
    u32     n_vendev;               /* element of the device_vendor */

    int (*pci_init)(struct pci_dev *dev);
} pci_drv_t;

typedef struct pci_dev {
    struct list_head devlist;

    pcicfgdata_t pcicfg;

    pci_drv_t *pcidrv;              /*  */

    u16 bar_io;
	u32 n_bar_mem;
    u32 bar_mem[5];

    void *param;
} pci_dev_t;

void getpciinfo(u32 bus, u32 dev, u32 func, pcicfgdata_t *pcicfg);
void pci_init(void);
void pci_dispcfgdata(pcicfgdata_t *pcicfg);
int pcidrv_register(pci_drv_t *drv);
int pcicfg_readw(pci_dev_t *pcidev, u32 off, u16 *val);
int pcicfg_writew(pci_dev_t *pcidev, u32 off, u16 val);


#endif

