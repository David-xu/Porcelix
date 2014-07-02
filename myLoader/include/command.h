#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "typedef.h"
#include "section.h"
#include "list.h"


struct command {
    char *cmd_name, *info;
    void *param;
    
    void (*op_func)(char *argv[], int argc, void *param);
};

void cmd_loop(void);


/*  */
extern unsigned n_command;
DEFINE_SYMBOL(cmddesc_array);
DEFINE_SYMBOL(cmddesc_array_end);



#endif

