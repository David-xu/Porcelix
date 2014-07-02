#ifndef _UDPCLI_H_
#define _UDPCLI_H_

#include "typedef.h"

enum {
    UDPCLI_CMDTYPE_REQ = 0,
    UDPCLI_CMDTYPE_ACK,
    UDPCLI_CMDTYPE_RETRY,
    UDPCLI_CMD_HDWRITE = 0x10,
};

typedef struct _udpcli_cmd {
    u8 cmd_ack;
    u8 cmdtype;
    u16 totallen;
    u32 id;
    union {
        struct {
            u8 hdidx;
            u32 logicsect;
        } hdwrite_s;
    }u;
    
    u8 buff[0];
} udpcli_cmd;

#endif

