#ifndef _RGMNG_H_
#define _RGMNG_H_

#include "avl.h"


typedef struct _rgnd {
	u32		begin, size;
} rgnd_t;

/* range resource management */
typedef struct _rgmng {
	u32		begin, size;
} rgmng_t;

#endif

