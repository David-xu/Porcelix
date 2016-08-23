#include "typedef.h"
#include "debug.h"
#include "list.h"
#include "io.h"
#include "ml_string.h"
#include "command.h"
#include "memory.h"
#include "section.h"
#include "hd.h"
#include "partition.h"
#include "module.h"
#include "net.h"
#include "task.h"
#include "userbin.h"

unsigned n_command = 0;


static void cmd_dispcpu_opfunc(char *argv[], int argc, void *param)
{
	u32 cur_cs, cur_ds, cur_es, cur_ss, cur_esp, cur_ebp;
	__asm__ __volatile__ (
		"movl	%%cs, %%eax			\n\t"
		"movl	%%eax, %0			\n\t"

		"movl	%%ds, %%eax			\n\t"
		"movl	%%eax, %1			\n\t"
		"movl	%%es, %%eax			\n\t"
		"movl	%%eax, %2			\n\t"

		"movl	%%ss, %%eax			\n\t"
		"movl	%%eax, %3			\n\t"

		"movl	%%esp, %4			\n\t"
		"movl	%%ebp, %5			\n\t"
		:"=m"(cur_cs), "=m"(cur_ds), "=m"(cur_es), 
		 "=m"(cur_ss), "=m"(cur_esp), "=m"(cur_ebp)
		:
		:"%eax"
	);

	printk("current regs:\n"
		   "cs: 0x%#4x, ds: 0x%#4x, es: 0x%#4x, ss: 0x%#4x\n"
		   "esp: 0x%#8x, ebp: 0x%#8x\n"
		   "eflags: 0x%#8x\n"
		   "CR0: 0x%#8x, CR3: 0x%#8x\n",
		   cur_cs, cur_ds, cur_es, cur_ss,
		   cur_esp, cur_ebp,
		   native_save_fl(),
		   getCR0(), getCR3());
}

struct command cmd_dispcpu _SECTION_(.array.cmd) =
{
    .cmd_name   = "dispcpu",
    .info       = "Display CPU info",
    .op_func    = cmd_dispcpu_opfunc,
};

/* 启动用户态线程例子 */
static int exec_test()
{
	struct pt_regs *regs = (struct pt_regs *)((u32)(current->stack) + TASKSTACK_SIZE) - 1;
	void *usermode_space = page_alloc(BUDDY_RANK_8K, MMAREA_NORMAL);
	regs->eip = 0x80000000;
	regs->xcs = USRDESC_CODE | 0x3;
	regs->eflags = EFLAGSMASK_IF | EFLAGSMASK_IOPL;
	regs->esp = 0x80000000 + 0x2000;
	regs->xss = USRDESC_DATA | 0x3;
	regs->xds = USRDESC_DATA | 0x3;
	regs->xes = USRDESC_DATA | 0x3;
	regs->xfs = USRDESC_DATA | 0x3;
	mmap(0x80000000, (u32)usermode_space, 0x2000, 1);
	memcpy(usermode_space, (void *)user_bin, userbin_len);
	return 0;
}

int testtask(void *param)
{
	printk("testtask()  0x%#8x\n", (u32)param);
	exec_test();
	return 0;
}

int testexit(void *param)
{
	printk("testexit() do nothing.  0x%#8x\n", (u32)param);
	return 0;
}

static void cmd_test_opfunc(char *argv[], int argc, void *param)
{
#if 0
	u32 base;
	char *name = allsym_resolvaddr(str2num(argv[1]), &base);
	printk("addr: 0x%#8x --> %s base 0x%#8x\n", str2num(argv[1]), name, base);

	kernel_thread(testtask, "testtask", (void *)0x12345678);
#else
	u16 tmp;
	outb(0x0c, 0x3d4);
	tmp = inb(0x3d5);
	tmp <<= 8;
	outb(0x0d, 0x3d4);
	tmp |= inb(0x3d5);
	printk("%#4x.\n", tmp);
#endif
}

struct command cmd_test _SECTION_(.array.cmd) =
{
    .cmd_name   = "test",
    .info       = "This is tEst function.",
    .op_func    = cmd_test_opfunc,
};

static void cmd_help_opfunc(char *argv[], int argc, void *param)
{
    unsigned i;
    struct command *cmd = (struct command *)param;
    /* list all cmd and their info string */
    if (argc == 1)
    {
        for (i = 0; i < n_command; i++, cmd++)
        {
            printk("%s\t%s\n", cmd->cmd_name, cmd->info);
        }
    }
}

struct command cmd_help _SECTION_(.array.cmd) =
{
    .cmd_name   = "help",
    .info       = "List all buildin commands.",
    .param      = cmddesc_array,
    .op_func    = cmd_help_opfunc,
};

static void cmd_parse_impl(char *cmd)
{
    char *argv[16] = {NULL};
    int i, argc = 0;
    
    argc = parse_str_by_inteflag(cmd, argv, 2, " \n");

    if (argc == 0)
        return;

    for (i = 0; i < n_command; i++)
    {
        struct command *cmd = (struct command *)cmddesc_array + i;
        u32 argv01_len = strlen(argv[0]);
        u32 cmd_len = strlen(cmd->cmd_name);
        if (argv01_len != cmd_len)
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

    printk("Known. Please input \'help\' to get help.\n");
}

static char* cmd_get_title(void)
{
    static char title[32];

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

int cmd_loop(void *param)
{
    int i, getch = 0;
    u16 pos = 0;
    char cmdBuff[256] = {0};

    while(1)
    {
        printk("%s", cmd_get_title());
        while (getch != '\n')
        {
        	/* task may block */
            getch = kbd_get_char();

            // backspace
            if (getch == '\b')
            {
                if (pos != 0)
                {
                    cmdBuff[pos] = 0;
                    pos--;
                    printk("%c", '\b');
                }
                continue;
            }

            // auto complete the cmd
            if (getch == '\t')
            {
                for (i = 0; i < n_command; i++)
                {
                    struct command *cmd = (struct command *)cmddesc_array + i;
                    char *cmd_name = cmd->cmd_name;
                    u16 cmdlen = strlen(cmd_name);

                    if (cmdlen < pos)
                        continue;
                    
                    if (0 == memcmp(cmdBuff, cmd_name, pos))
                    {
                        // cp the left cmdname
                        memcpy(&cmdBuff[pos], &(cmd_name[pos]), cmdlen - pos);
                        printk("%s ", &(cmd_name[pos]));
                        pos = cmdlen;
                        cmdBuff[pos] = ' ';
                        pos++;
                        break;
                    }
                }
                continue;
            }
            
            cmdBuff[pos++] = getch;
            printk("%c", getch);
        }
        
        /* parse and process the cmd */
        cmd_parse_impl(cmdBuff);

        memset(cmdBuff, 0, sizeof(cmdBuff));

        pos = 0;
        getch = 0;
    }

    /* never reach here */
    return 0;
}

static void __init cmdlist_init(void)
{
    n_command = (GET_SYMBOLVALUE(cmddesc_array_end) - GET_SYMBOLVALUE(cmddesc_array)) / sizeof(struct command);

    /* this is the command loop thread */
    kernel_thread(cmd_loop, "cmdline", NULL);
}

module_init(cmdlist_init, 7);


