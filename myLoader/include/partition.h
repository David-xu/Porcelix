#ifndef _PARTITION_H_
#define _PARTITION_H_

#include "fs.h"
#include "device.h"

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

