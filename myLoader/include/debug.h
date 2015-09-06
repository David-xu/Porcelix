#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "config.h"
#include "typedef.h"

#if CONFIG_DBG_SWITCH
#define DEBUG(fmt, ...)     printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG(fmt, ...)
#endif

#if HDDRV_DBG_SWITCH
#define DEBUG_HD(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_HD(fmt, ...)
#endif

void dump_stack(u32 ebp);
void dump_ram(void *addr, u16 len);

#endif

