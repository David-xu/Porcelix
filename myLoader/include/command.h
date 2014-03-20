#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "typedef.h"
#include "section.h"
#include "list.h"


struct command{
    u8 *cmd_name, *info;
    void *param;
    
    void (*op_func)(u8 *argv[], u8 argc, void *param);
};

void cmdlist_init(void) _SECTION_(.init.text);

/*  */
extern struct command cmddesc_array[];
extern unsigned n_command;
DEFINE_SYMBOL(cmddesc_array_end);


#endif

