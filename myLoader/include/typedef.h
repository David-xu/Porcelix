#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

#include "config.h"

#define TRUE            (1)
#define FALSE           (0)

#define IN
#define OUT
#define INOUT

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
#ifdef CONFIG_32
typedef unsigned long long  u64;
#else
typedef unsigned long       u64;
#endif


typedef char                s8;
typedef short               s16;
typedef int                 s32;
#ifdef CONFIG_32
typedef long long           s64;
#else
typedef long                s64;
#endif

typedef enum {
    NOERR = 0,
    UNKNOW,
    NORESOUCE,
    COUNT
}RETYPE;

#define NULL        ((void *)0)

#endif

