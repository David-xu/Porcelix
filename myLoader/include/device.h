#ifndef _DEVICE_H_
#define _DEVICE_H_

enum dev_type{
    DEVTYPE_NORMAL = 0,
    DEVTYPE_BLOCK,
};

struct _device;
typedef struct _device_driver{
    int (*read)(struct _device *dev, u32 addr, void *buff, u32 size);
    int (*write)(struct _device *dev, u32 addr, void *buff, u32 size);

    void *drv_param;
}device_driver_t;

typedef struct _device{
    u8 *name;
    enum dev_type   device_type;
    device_driver_t *driver;            /*  */

    void *dev_param;                    /* some device private paramters */
}device_t;

#endif
