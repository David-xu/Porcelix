#include "config.h"
#include "typedef.h"
#include "public.h"
#include "module.h"
#include "io.h"

moduleinitlist_t moduleinitlist[] =
{
    {GET_SYMBOLVALUE(moduleinit_array_0), GET_SYMBOLVALUE(moduleinit_array_0_end)},
    {GET_SYMBOLVALUE(moduleinit_array_1), GET_SYMBOLVALUE(moduleinit_array_1_end)},
    {GET_SYMBOLVALUE(moduleinit_array_2), GET_SYMBOLVALUE(moduleinit_array_2_end)},
    {GET_SYMBOLVALUE(moduleinit_array_3), GET_SYMBOLVALUE(moduleinit_array_3_end)},
    {GET_SYMBOLVALUE(moduleinit_array_4), GET_SYMBOLVALUE(moduleinit_array_4_end)},
    {GET_SYMBOLVALUE(moduleinit_array_5), GET_SYMBOLVALUE(moduleinit_array_5_end)},
    {GET_SYMBOLVALUE(moduleinit_array_6), GET_SYMBOLVALUE(moduleinit_array_6_end)},
    {GET_SYMBOLVALUE(moduleinit_array_7), GET_SYMBOLVALUE(moduleinit_array_7_end)},
};

/*
 * call the init func with each module from level 0 to level 7
 */
void init_module(void)
{
    u32 *p, i, initfuncstat[ARRAY_ELEMENT_SIZE(moduleinitlist)];
    moduleinit init;

    for (i = 0; i < ARRAY_ELEMENT_SIZE(moduleinitlist); i++)
    {
        initfuncstat[i] = 0;
        for (p = (u32 *)(moduleinitlist[i].begin);
             p < (u32 *)(moduleinitlist[i].end);
             p++)
        {
             init = (moduleinit)(*p);
             init();
             initfuncstat[i]++;
        }
    }
    
    printf("module init count:");
    for (i = 0; i < ARRAY_ELEMENT_SIZE(moduleinitlist); i++)
    {
        if ((i % 4) == 0)
        {
            printf("\n");
        }
        printf("lvl(%d):%#2x ", i, initfuncstat[i]);
    }
    printf("\n");
}

