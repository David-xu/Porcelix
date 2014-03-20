#ifndef _HDREG_H_
#define _HDREG_H_

#include "typedef.h"

#define PORT_HD_DATA                (0x01F0)
#define PORT_HD_ERR_PRECOMP         (0x01F1)
#define PORT_HD_NSECTOR             (0x01F2)
#define PORT_HD_SECTOR              (0x01F3)    /* start sector num */
#define PORT_HD_LCYL                (0x01F4)    /* low byte of the cylinder */
#define PORT_HD_HCYL                (0x01F5)    /* high byte of the cylinder */
#define PORT_HD_CURRENT             (0x01F6)
#define PORT_HD_STAT_COMMAND        (0x01F7)
#define PORT_HD_CMD                 (0x03F6)
#if 0
#define PORT_HD_                    (0x03F7)
#endif

/* the status mask, read the PORT:PORT_HD_STAT_COMMAND */
#define HD_STATMASK_ERR_STAT        (0x01)
#define HD_STATMASK_INDEX_STAT      (0x02)
#define HD_STATMASK_ECC_STAT        (0x04)
#define HD_STATMASK_DRQ_STAT        (0x08)
#define HD_STATMASK_SEEK_STAT       (0x10)
#define HD_STATMASK_WRERR_STAT      (0x20)
#define HD_STATMASK_READY_STAT      (0x40)
#define HD_STATMASK_BUSY_STAT       (0x80)

/* hd cmd */
#define HD_CMD_RESTORE              (0x10)
#define HD_CMD_READ                 (0x20)
#define HD_CMD_WRITE                (0x30)
#define HD_CMD_VERIFY               (0x40)
#define HD_CMD_FORMAT               (0x50)
#define HD_CMD_INIT                 (0x60)
#define HD_CMD_SEEK                 (0x70)
#define HD_CMD_DIAGNOSE             (0x90)
#define HD_CMD_SPECIFY              (0x90)


#endif

