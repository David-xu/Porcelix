#include "typedef.h"
#include "debug.h"
#include "list.h"
#include "io.h"
#include "string.h"
#include "command.h"
#include "memory.h"
#include "section.h"
#include "hd.h"

unsigned n_command = 0;

static void cmd_test_opfunc(u8 *argv[], u8 argc, void *param)
{
    /* show the left ram space */
    freepage_dump(TRUE);
}

struct command
{
    .cmd_name   = "test",
    .info       = "This is tEst function.",
    .op_func    = cmd_test_opfunc,
};

static void cmd_help_opfunc(u8 *argv[], u8 argc, void *param)
{
    unsigned i;
    struct command *cmd = (struct command *)param;
    /* list all cmd and their info string */
    if (argc == 1)
    {
        for (i = 0; i < n_command; i++, cmd++)
        {
            printf("%s\t%s\n", cmd->cmd_name, cmd->info);
        }
    }
}

struct command
{
    .cmd_name   = "help",
    .info       = "List all buildin commands.",
    .param      = cmddesc_array,
    .op_func    = cmd_help_opfunc,
};



void cmdlist_init(void)
{
    n_command = (GET_SYMBOLVALUE(cmddesc_array_end) - (unsigned)cmddesc_array) / sizeof(struct command);
}

static void cmd_parse_impl(u8 *cmd)
{
    u8 *argv[16] = {NULL}, argc = 0;
    int i, flag = 0;
    
    argc = parse_str_by_inteflag(cmd, argv, 2, " \n");

    if (argc == 0)
        return;

    for (i = 0; i < n_command; i++)
    {
        struct command *cmd = cmddesc_array + i;
        u32 argv01_len = strlen(argv[0]);
        u32 cmd_len = strlen(cmd->cmd_name);
        if (argv01_len > cmd_len)
            continue;
        if (0 == memcmp(argv[0], cmd->cmd_name, argv01_len))
        {
            if (cmd->op_func != NULL)
            {
                cmd->op_func(argv, argc, cmd->param);
            }
            return;
        }
    }

    printf("Known. Please input \'help\' to get help.\n");
}

static u8* cmd_get_title()
{
    static u8 title[32];

    if (cursel_partition == NULL)
    {
        sprintf(title, "%s", "mld:>");
    }
    else
    {
        sprintf(title, "HD(0,%d):%s$",
                cursel_partition->part_idx,
                cursel_partition->fs->fs_getcurdir(cursel_partition->fs, cursel_partition));
    }
    
    return title;
}

void cmd_loop()
{
    int i, getch = 0;
    u16 pos = 0;
    u8 cmdBuff[256] = {0};
    
    while(1)
    {
        printf("%s", cmd_get_title());
        while (getch != '\n')
        {
            getch = kbd_get_char();
            if (getch == -1)
                continue;

            // backspace
            if (getch == '\b')
            {
                if (pos != 0)
                {
                    cmdBuff[pos] = 0;
                    pos--;
                    printf("%c", '\b');
                }
                continue;
            }

            // auto complete the cmd
            if (getch == '\t')
            {
                for (i = 0; i < n_command; i++)
                {
                    struct command *cmd = cmddesc_array + i;
                    u8* cmd_name = cmd->cmd_name;
                    u16 cmdlen = strlen(cmd_name);

                    if (cmdlen < pos)
                        continue;
                    
                    if (0 == memcmp(cmdBuff, cmd_name, pos))
                    {
                        // cp the left cmdname
                        memcpy(&cmdBuff[pos], &(cmd_name[pos]), cmdlen - pos);
                        printf("%s ", &(cmd_name[pos]));
                        pos = cmdlen;
                        cmdBuff[pos] = ' ';
                        pos++;
                        break;
                    }
                }
                continue;
            }
            
            cmdBuff[pos++] = getch;
            printf("%c", getch);
        }
        
        /* parse and process the cmd */
        cmd_parse_impl(cmdBuff);

        memset(cmdBuff, 0, sizeof(cmdBuff));

        pos = 0;
        getch = 0;
    }
}

