#ifndef _ASSERT_H_
#define _ASSERT_H_

#include "config.h"
#include "io.h"

void die(void);

#define ASSERT(cond)                                                \
        {                                                           \
            if (!(cond))                                            \
            {                                                       \
                printf("ASSERT!\nFile:%s, Func:%s, Line:%d\n%s\n",  \
                       __FILE__, __func__, __LINE__, #cond);        \
                die();												\
            }                                                       \
        }

#define	ERROR(fmt, ...)												\
		{															\
			printf("ERROR!\nFile:%s, Func:%s, Line:%d\n",			\
				   __FILE__, __func__, __LINE__);					\
			printf(fmt, ##__VA_ARGS__);								\
			die();													\
		}

#endif

