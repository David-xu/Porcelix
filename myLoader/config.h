#ifndef _CONFIG_H_
#define _CONFIG_H_

#define CONFIG_32
// #define CONFIG_64

#define HD_SECTOR_SIZE              (512)

/******************************************************************************/
/*                   used for loader                                          */
/******************************************************************************/
/* core begin sector num  */
#define IMGCORE_OFFSET              (1)
/* core img load addr */
#define	IMGCORE_LOADADDR_OFFSET     (0x8000)        /* offset */
#define IMGCORE_LOADADDR_BASE		(0x2000)        /* ds */
#define IMGCORE_LOADADDR			((IMGCORE_LOADADDR_BASE << 16) + IMGCORE_LOADADDR_OFFSET)
/* sector number of the core */
#define SECTORNUM_CORESIZE          (255)
/* coresize */
#define IMGCORE_SIZE_WORD           (HD_SECTOR_SIZE * SECTORNUM_CORESIZE / 2)
/* hd pdt and hd info position */
#define HD_INFOADDR_ENTRYADDR_BASE      (0x0000)
#define HD_INFOADDR_ENTRYADDR_OFFSET    (0x0104)
#define HD_PDTADDR_BASS                 (0x07C0)
#define HD_PDTADDR_OFFSET               (0x01BE)



/* segment desc */
#define SYSDESC_CODE                (0x10)
#define SYSDESC_DATA                (0x18)

/* system init sp */
#define SYSINIT_SP                  (0xA0000)


/* debug switch */
#define CONFIG_DBG_SWITCH           (0)

/******************************************************************************
 *                           Used for MM                                      *
 ******************************************************************************/
#define MM_HOLEAFTEL_SYSRAM         (2048)

/******************************************************************************
 *                           Used for FS                                      *
 ******************************************************************************/
#define CONFIG_FSDEBUG_DUMP         (0)

#endif

