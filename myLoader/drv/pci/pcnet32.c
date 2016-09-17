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
#define PCI_VENDOR_ID_AMD			0x1022
#define PCI_DEVICE_ID_AMD_PCNET		0x2000

static int amdpcnet_devinit(struct pci_dev *dev)
{
	
	return 0;
}

static vendor_device_t spt_vendev[] = {{PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_PCNET},};

static pci_drv_t amdpcnet_drv = {
    .drvname = "amdpcnet_drv",
    .vendev = spt_vendev,
    .n_vendev = ARRAY_ELEMENT_SIZE(spt_vendev),
    .pci_init = amdpcnet_devinit,
};


/* this is the driver for realtek ethernet chip 10EC:8139 */
static void __init amdpcnetinit()
{
    /* register the pci drv */
    pcidrv_register(&amdpcnet_drv);
}

module_init(amdpcnetinit, 5);
