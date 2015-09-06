#ifndef _TASK_H_
#define _TASK_H_

#include "typedef.h"
#include "list.h"
#include "memory.h"
#include "section.h"

#define TASKSTACK_RANGE         BUDDY_RANK_8K
#define TASKSTACK_SIZE          (PAGE_SIZE << TASKSTACK_RANGE)

typedef enum _task_state {
    STATE_RUNNING = 0,
    STATE_WAIT,
    STATE_STOP,
    STATE_EXIT
} task_state_e;

#define		TASKFLAG_KT				(0x00000001)			/* Kernel thread */

struct _task_stack;
typedef struct _task {
	task_state_e    state;
	int     pid;
	u32		tskflag;

	u32     ctx_ip, ctx_sp;

	u32		pgd_pa;				/* task page directory phy addr */

	struct list_head q;			/* link this task desc into q */

	struct _task_stack *stack;
} task_t;

typedef struct _tss {
	unsigned short		back_link, __blh;
	unsigned long		sp0;
	unsigned short		ss0, __ss0h;
	unsigned long		sp1;
	/* ss1 caches MSR_IA32_SYSENTER_CS: */
	unsigned short		ss1, __ss1h;
	unsigned long		sp2;
	unsigned short		ss2, __ss2h;
	unsigned long		__cr3;
	unsigned long		ip;
	unsigned long		flags;
	unsigned long		ax;
	unsigned long		cx;
	unsigned long		dx;
	unsigned long		bx;
	unsigned long		sp;
	unsigned long		bp;
	unsigned long		si;
	unsigned long		di;
	unsigned short		es, __esh;
	unsigned short		cs, __csh;
	unsigned short		ss, __ssh;
	unsigned short		ds, __dsh;
	unsigned short		fs, __fsh;
	unsigned short		gs, __gsh;
	unsigned short		ldt, __ldth;
	unsigned short		trace;			/* LSB indicate the 'Debug Trace' */
	unsigned short		io_bitmap_base;
} tss_t;

asmlinkage task_t *__switch_to(task_t *prev, task_t *next);

typedef struct _task_stack {
    task_t *task;
} task_stack_t;

static inline task_stack_t *getcurstack(void)
{
    u32 esp;
    asm volatile (
        "movl   %%esp, %0       \n\t"
        : "=m"(esp)
        :
        : "memory"
    );
    return (task_stack_t *)(esp & (~(TASKSTACK_SIZE - 1)));
}

#define switch_to(prev, next)                   \
    do {                                        \
        unsigned long ebx, ecx, edx, esi, edi;  \
        asm volatile (                          \
            "pushfl                     \n\t"   \
            "pushl  %%ebp               \n\t"   \
            /* save the prev eip and esp */     \
            "movl   $after, %[prev_ip]  \n\t"   \
            "movl   %%esp, %[prev_sp]   \n\t"   \
            /* restore the next esp */          \
            "movl   %[next_sp], %%esp   \n\t"   \
            /* push the next ip and ret */      \
            "pushl  %[next_ip]          \n\t"   \
            "jmp    __switch_to         \n\t"   \
            "after:                     \n\t"   \
            "popl   %%ebp               \n\t"   \
            "popfl"                             \
            : [prev_ip]"=m"(prev->ctx_ip),      \
              [prev_sp]"=m"(prev->ctx_sp),      \
              "=b"(ebx), "=c"(ecx), "=d"(edx),  \
              "=S"(esi), "=D"(edi)              \
            : [next_ip]"m"(next->ctx_ip),       \
              [next_sp]"m"(next->ctx_sp),       \
              "a"(prev), "d"(next)              \
            : "memory"                          \
        );                                      \
    } while (0)

#define current         (getcurstack()->task)

void schedule(void);
void tail_sched(void);
void exit(int exit_code);

typedef asmlinkage int (*task_entry)(struct pt_regs *regs, void *param);

/* ring 0 thread */
int kernel_thread(task_entry entry, void *param);
int exec_test();

void idleloop(void);

void kt_after_switch(void);


/* sleep
 * usec: micro second
 */
void sleep(int usec);
void wait_ms(u32 ms);			/* busy loop */

extern volatile u32	smp_nap;

#endif

