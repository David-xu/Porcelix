#include "typedef.h"
#include "assert.h"
#include "section.h"
#include "desc.h"
#include "io.h"
#include "hd.h"
#include "debug.h"
#include "hdreg.h"
#include "memory.h"

#if CONFIG_DBG_SWITCH

struct test_st{
    u8  *name;
    struct list_head inst;
};

#define TEST_FUNC_STDEF(namestr)            \
    struct test_st namestr =                \
            {.name = #namestr,              \
            }

void test()
{
    struct test_st *p;
    LIST_HEAD(listhead);
    TEST_FUNC_STDEF(test1);
    TEST_FUNC_STDEF(test2);
    TEST_FUNC_STDEF(test3);
    TEST_FUNC_STDEF(test4);
    list_add_tail(&(test1.inst), &listhead);
    list_add_tail(&(test3.inst), &listhead);
    list_add_tail(&(test4.inst), &listhead);
    list_add_tail(&(test2.inst), &listhead);

    LIST_FOREACH_ELEMENT(p, &listhead, inst)
    {
        printf("%s\n", p->name);
    }
}
#else
void test()
{

}
#endif

// void loader_entry(void) __attribute__((noreturn));

void loader_entry(void)
{
    interrupt_init();
    disp_init();            /* after that we can do some screen print... */

    printf("********************************************************************************");
    printf("*                                                                              *");
    printf("*              This is myLoader, all rights reserved by David.                 *");
    printf("*                                                                              *");
    printf("********************************************************************************\n");
    printf("System Begin...\n");

    /*  */
    kbd_init();
    hd_init();

    /* build the free page buddy system. */
    mem_init();

    /* build the file system, try to load the main fs */
    fs_init();

    /* cmd init */
    cmdlist_init();

    test();

    cmd_loop();
}

