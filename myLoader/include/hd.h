#ifndef _HD_H_
#define _HD_H_

#include "typedef.h"
#include "fs.h"
#include "device.h"

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



/* the hd partion table store in address: 0x7c00 + 0x01BE */
struct hd_dptentry{
    u8      boot_ind;
    u8      start_head;
    u16     start_sect: 6;
    u16     start_cyl : 10;
    u8      sys_ind;    /* 0xb-DOS, 0x80-OldMinix, 0x83-Linux */
    u8      end_head;
    u16     end_sect: 6;
    u16     end_cyl : 10;
    u32     start_logicsect;        /* this is the partition start logicsect
                                       in whole harddisk sector range */
    u32     n_logicsect;
}__attribute__((packed));

#define HD_PDTENTRY_NUM             (4)
extern struct hd_dptentry hd0_pdt[HD_PDTENTRY_NUM];

/* this is the system type enum */
enum {
    SYSTYPE_MSDOS       = 0x0b,
    SYSTYPE_OLDMINIX    = 0x80,
    SYSTYPE_LINUX       = 0x83,
};

/* this is the partition define struct.
 * 
 */
struct partition_desc{
    unsigned part_idx;
    struct file_system *fs;         /* each partition mounted by one type of the fs.
                                     * before the mount op, this field is NULL.
                                     * after the umount op, this field is NULL.
                                     */
    device_t *dev;
    /* some partition dependent data... */
    void *param;
};
extern struct partition_desc hdpart_desc[];

/* this is the cur selected partition. */
extern struct partition_desc *cursel_partition;

#endif


