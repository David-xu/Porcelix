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
#define IMGCORE_LOADADDR			(0x30000)
#define	IMGCORE_LOADADDR_OFFSET     (IMGCORE_LOADADDR & 0xF)				/* offset */
#define IMGCORE_LOADADDR_BASE		(IMGCORE_LOADADDR >> 4)					/* ds */

#if 1
/* int 0x13 al=0x42
   max sector number:007Fh   for   Phoenix   EDD */
#define SECTORNUM_CORESIZE          (127)
/* coresize */
#define IMGCORE_SIZE_WORD           (HD_SECTOR_SIZE * SECTORNUM_CORESIZE / 2)
#endif

/* hd pdt and hd info position */
#define HD0_INFOADDR_ENTRYADDR_BASE      (0x0000)
#define HD0_INFOADDR_ENTRYADDR_OFFSET    (0x0104)
#define HD1_INFOADDR_ENTRYADDR_BASE      (0x0000)
#define HD1_INFOADDR_ENTRYADDR_OFFSET    (0x0118)


//
#define BOOTPARAM_PACKADDR_BASS         (0x07C0)
#define BOOTPARAM_PACKADDR_OFFSET       (0x01A0)
#define BOOTPARAM_PACKADDR_LEN          (0x60)



/* segment desc */
#define SYSDESC_CODE                (0x10)
#define SYSDESC_DATA                (0x18)

/* system init sp */
#define SYSINIT_SP                  (0xA0000)


/* debug switch */
#define CONFIG_DBG_SWITCH           (0)

/******************************************************************************
 *                           Used for HD driver                               *
 ******************************************************************************/
#define HDDRV_DBG_SWITCH            (0)

/******************************************************************************
 *                           Used for MM                                      *
 ******************************************************************************/
#define MM_HOLEAFTEL_SYSRAM         (2048)

/******************************************************************************
 *                           Used for FS                                      *
 ******************************************************************************/
#define CONFIG_FSDEBUG_DUMP         (0)

/******************************************************************************
 *                           Used for net                                     *
 ******************************************************************************/
#define CONFIG_UDPCLI_DEFAULTPORT   (7788)
#define CONFIG_UDPCLI_HELLO_REQ     "hello"
#define CONFIG_UDPCLI_HELLO_ACK     "i'm myloader."
#define CONFIG_UDPCLI_BYE_REQ       "bye"
#define CONFIG_UDPCLI_BYE_ACK       "bye"


#endif

