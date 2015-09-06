#ifndef _SYSCALL_H_
#define _SYSCALL_H_

asmlinkage void syscall_pubentry(struct pt_regs *regs, u32 syscall_num);

#endif
