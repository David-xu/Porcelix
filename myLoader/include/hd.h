#ifndef _HD_H_
#define _HD_H_

#include "typedef.h"
#include "device.h"
#include "boot.h"

enum hdreq_e{
    HDREQ_READ = 0,
    HDREQ_WRITE,
};

struct hd_request{
    void *buff;
    
    u16 sect_num;
    
    u16 cyl_start;
    u8 head_start;
    u8 sect_start;

    enum hdreq_e hdreq;
    u8 volatile flag;
};

/* the hd infomation stored at aaaa:bbbb 0000:0104
 * and the value of aaaa:bbbb stored at one of the int entry 0000:0104
 * first : get the addr aaaa:bbbb from 0000:0104
 * second: get the hd info from the address of aaaa:bbbb
 */
struct hd_info{
    u16     n_cyl;          /* cyl num */
    u8      n_header;       /* header num */
    u16     rsv0;
    u16     pw_comp;        /* prewrite compensation */
    u8      rsv1;
    u8      ctrl;
    u8      rsv2[3];
    u16     stop_cyl;
    u8      n_sect;         /* sect num */
    u8      rsv3;
}__attribute__((packed));

// extern struct hd_info hdinfo;



#define HD_PDTENTRY_NUM             (4)
extern struct hd_dptentry hd0_pdt[HD_PDTENTRY_NUM];

/* this is the system type enum */
enum {
    SYSTYPE_MSDOS       = 0x0b,
    SYSTYPE_OLDMINIX    = 0x80,
    SYSTYPE_LINUX       = 0x83,
};

/* direct hd operation interface */
#define HDPART_HD0_WHOLE      (0xF0)
#define HDPART_HD1_WHOLE      (0xF1)

device_t *gethd_dev(u8 part);

#endif


