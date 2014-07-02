#ifndef _MODULE_H_
#define _MODULE_H_

#include "section.h"

#define MODULEINIT_MAXLEVEL        (8)

DEFINE_SYMBOL(moduleinit_array_0);
DEFINE_SYMBOL(moduleinit_array_0_end);
DEFINE_SYMBOL(moduleinit_array_1);
DEFINE_SYMBOL(moduleinit_array_1_end);
DEFINE_SYMBOL(moduleinit_array_2);
DEFINE_SYMBOL(moduleinit_array_2_end);
DEFINE_SYMBOL(moduleinit_array_3);
DEFINE_SYMBOL(moduleinit_array_3_end);
DEFINE_SYMBOL(moduleinit_array_4);
DEFINE_SYMBOL(moduleinit_array_4_end);
DEFINE_SYMBOL(moduleinit_array_5);
DEFINE_SYMBOL(moduleinit_array_5_end);
DEFINE_SYMBOL(moduleinit_array_6);
DEFINE_SYMBOL(moduleinit_array_6_end);
DEFINE_SYMBOL(moduleinit_array_7);
DEFINE_SYMBOL(moduleinit_array_7_end);

typedef void (*moduleinit)();

typedef struct moduleinitlist {
    u32 begin, end;
} moduleinitlist_t;

#define module_init(func, level)                            \
        static moduleinit __used func##modinit##level __attribute__((__section__(".array.moduleinit_"#level))) = (func);

void init_module(void);


#endif

