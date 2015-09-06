#include "public.h"
#include "interrupt.h"
#include "io.h"
#include "task.h"

asmlinkage void syscall_pubentry(struct pt_regs *regs, u32 syscall_num)
{
	printf("syscall %d...\n", syscall_num);
	switch (syscall_num)
	{
		case 0x10:

		{
			printf("%s\n", regs->ebx);
			break;
		}
		case 0x12:
		{
			conspc_printf("%c", regs->ebx);
			break;
		}
		default:
		{
			printf("unkown syscall from task %d\n", current->pid);
			dump_ptregs(regs);
		}
	}
}

