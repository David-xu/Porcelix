#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "config.h"
#include "typedef.h"

#if CONFIG_DBG_SWITCH
#define DEBUG(fmt, ...)			printk(fmt, ##__VA_ARGS__)
#else
#define DEBUG(fmt, ...)
#endif

#if HDDRV_DBG_SWITCH
#define DEBUG_HD(fmt, ...)		printk(fmt, ##__VA_ARGS__)
#else
#define DEBUG_HD(fmt, ...)
#endif

#if SCHED_DBG_SWITCH
#define DEBUG_SCHED(fmt, ...)	printk(fmt, ##__VA_ARGS__)
#else
#define DEBUG_SCHED(fmt, ...)
#endif

#if NET_DBG_SWITCH
#define DEBUG_NET(fmt, ...)		printk(fmt, ##__VA_ARGS__)
#else
#define DEBUG_NET(fmt, ...)
#endif


void dump_stack(u32 ebp);
void dump_ram(void *addr, u16 len);

#endif

