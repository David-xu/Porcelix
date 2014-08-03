#ifndef _ASSERT_H_
#define _ASSERT_H_

#include "config.h"
#include "io.h"

#define ASSERT(cond)                                                \
        {                                                           \
            if (!(cond))                                            \
            {                                                       \
                printf("ASSERT!\nFile:%s, Func:%s, Line:%d\n%s\n",  \
                       __FILE__, __func__, __LINE__, #cond);        \
                while(1);                                           \
            }                                                       \
        }

#define	ERROR(fmt, ...)												\
		{															\
			printf("ERROR!\nFile:%s, Func:%s, Line:%d\n",			\
				   __FILE__, __func__, __LINE__);					\
			printf(fmt, ##__VA_ARGS__);								\
			while(1);												\
		}

#endif

