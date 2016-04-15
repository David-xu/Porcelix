#include "config.h"

.global thread_start
.global tail_sched
.global exit
.global die

/* 我们仿照linux的调用栈保存方式 即产生exception时的现场会保存成如下的样子

 *	40(%esp) - %oldss
 *	3C(%esp) - %oldesp
 *	38(%esp) - %eflags
 *	34(%esp) - %cs
 *	30(%esp) - %eip
 *	2C(%esp) - orig_eax			错误码或者系统调用号							中断返回需要跳过
 *	28(%esp) - %gs				实际处理函数入口 由各个中断入口处理代码自行设置	中断返回需要跳过
 *	24(%esp) - %fs
 *	20(%esp) - %es
 *	1C(%esp) - %ds
 *	18(%esp) - %eax
 *	14(%esp) - %ebp
 *	10(%esp) - %edi
 *   C(%esp) - %esi
 *	 8(%esp) - %edx
 *	 4(%esp) - %ecx
 *	 0(%esp) - %ebx

   如果是RING3调用int 0x80进内核 那么需要将eax中的系统调用号保存到orig_eax位置 
   如果是RING3发生异常进内核 则orig_eax位置会是x86 arch压入栈中的error_code 对于arch不产生error_code的 入口手动压入0是的栈结构保持一致
   如果是RING0产生异常 则顶部两个oldss 和oldesp不存在 同样需要根据异常类型考虑是否压入0作为error_code的填充
   */

#define	SAVE_ALL			\
	pushl	%fs;			\
	pushl	%es;			\
	pushl	%ds;			\
	pushl	%eax;			\
	pushl	%ebp;			\
	pushl	%edi;			\
	pushl	%esi;			\
	pushl	%edx;			\
	pushl	%ecx;			\
	pushl	%ebx;
	
#define RESTORE_ALL			\
	popl	%ebx;			\
	popl	%ecx;			\
	popl	%edx;			\
	popl	%esi;			\
	popl	%edi;			\
	popl	%ebp;			\
	popl	%eax;			\
	popl	%ds;			\
	popl	%es;			\
	popl	%fs

.global intprocjmp_master
.global intprocjmp_slaver


#define INTENTRY_8259MASTER(irq)			\
.global	intproc_##irq;						\
intproc_##irq:								\
	pushl	$##irq;							\
	pushl	$intprocjmp_master;				\
	jmp		exp_pubpath
	
	
#if 0
	pushl	%eax;							\
	pushl	$intprocjmp_##irq;				\
	SAVE_ALL;								\
	movl	0x28(%esp), %edi;				\
	movl	0x2c(%esp), %edx;				\
	movl	%esp, %eax;						\
	call	*%edi;							\
	call	_8259master_EOI;				\
	call	tail_sched;	/* 启动一次调度 */	\
	RESTORE_ALL;							\
	addl	$8, %esp;						\
	iret
#endif

INTENTRY_8259MASTER(0)
INTENTRY_8259MASTER(1)
INTENTRY_8259MASTER(2)
INTENTRY_8259MASTER(3)
INTENTRY_8259MASTER(4)
INTENTRY_8259MASTER(5)
INTENTRY_8259MASTER(6)
INTENTRY_8259MASTER(7)

#define INTENTRY_8259SLAVER(irq)			\
.global	intproc_##irq;						\
intproc_##irq:								\
	pushl	$##irq;							\
	pushl	$intprocjmp_slaver;				\
	jmp		exp_pubpath

#if 0
	pushl	%eax;							\
	pushl	$intprocjmp_##irq;				\
	SAVE_ALL;								\
	movl	0x28(%esp), %edi;				\
	movl	0x2c(%esp), %edx;				\
	movl	%esp, %eax;						\
	call	*%edi;							\
	call	_8259slaver_EOI;				\
	call	tail_sched;	/* 启动一次调度 */	\
	RESTORE_ALL;							\
	addl	$8, %esp;						\
	iret
#endif

INTENTRY_8259SLAVER(8)
INTENTRY_8259SLAVER(9)
INTENTRY_8259SLAVER(10)
INTENTRY_8259SLAVER(11)
INTENTRY_8259SLAVER(12)
INTENTRY_8259SLAVER(13)
INTENTRY_8259SLAVER(14)
INTENTRY_8259SLAVER(15)


.global lapictimer_entrance
.global lapictimer
lapictimer_entrance:
	pushl	%eax
	pushl	$lapictimer
	jmp		exp_pubpath

.global syscall_entry
.global syscall_pubentry
syscall_entry:
	pushl	%eax					/* 系统调用号 */
	pushl	$syscall_pubentry		/* 系统调用统一入口 */
	jmp		exp_pubpath

.global NMIfault_entry
.global do_nmifault
NMIfault_entry:
	pushl	$0
	pushl	$do_nmifault			/* 实际的异常处理入口 */
	jmp		exp_pubpath


.global DFfault_entry
.global do_dffault
DFfault_entry:
	pushl	$do_dffault				/* 实际的异常处理入口 */
	jmp		exp_pubpath

/* some fault entry */
.global TSSfault_entry
.global do_tssfault
TSSfault_entry:
	pushl	$do_tssfault			/* 实际的异常处理入口 */
	jmp		exp_pubpath

.global NPfault_entry
.global do_npfault
NPfault_entry:
	pushl	$do_npfault				/* 实际的异常处理入口 */
	jmp		exp_pubpath
	
.global SSfault_entry
.global do_ssfault
SSfault_entry:
	pushl	$do_ssfault				/* 实际的异常处理入口 */
	jmp		exp_pubpath
	
.global GPfault_entry
.global do_gpfault
GPfault_entry:
	pushl	$do_gpfault				/* 实际的异常处理入口 */
	jmp		exp_pubpath



.global PFfault_entry
.global do_pagefault
PFfault_entry:
	pushl	$do_pagefault			/* 实际的中断处理入口 */
	
	/* 所有的中断(包括异常和陷阱)入口全部走这个路径 */
exp_pubpath:						
	SAVE_ALL

	/*  */
	movl	$SYSDESC_DATA, %eax
	movw	%ax, %ds
	movw	%ax, %es
					
	movl	0x14(%esp), %ebp		/* 0x14(%esp) 保存的是当前的%ebp值 */
	movl	0x28(%esp), %edi		/* 0x28(%esp)的位置上存着实际处理入口函数, 是在实际的中断入口压入的处理地址 */
	movl	0x2c(%esp), %edx		/* 0x2c(%esp)位置存放调用参数, 可能是以下几种可能
									 * 1. error code, 针对异常处理
									 * 2. irq, 针对中断
									 * 3. syscall number, 针对int 0x80的syscall陷入
									 */
	movl	%esp, %eax				/* */
	call	*%edi					/* 执行实际处理函数 */
	call	tail_sched				/* 启动一次尾调度 */
	/*
	 * 该处为线程启动点, 调度器切换到新建的线程时会以此为入口, 
	 * 普通线程和内核线程的区别在与构造的中断返回调用栈不通
	 */
thread_start:
	RESTORE_ALL
	addl	$8, %esp				/* skip the %gs and orig_code */

	iret
