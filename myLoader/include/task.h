#ifndef _TASK_H_
#define _TASK_H_

#include "typedef.h"
#include "list.h"
#include "memory.h"
#include "section.h"
#include "spinlock.h"

typedef struct _wait_queue {
	/* wait list head */
	struct list_head	wlh;
	/* wait queue locker */
	spinlock_t			wlock;
} wait_queue_t;

struct _task;

typedef enum _wakeup_reason {
	WU_REASON_TIMEOUT = 0,
	WU_REASON_WAKEUP,
	WU_REASON_NUM
} wakeup_reason_e;

#define	TIMEOUT_INFINIT			0xFFFFFFFF

typedef struct _wait_task {
	wait_queue_t		*wq;			/* 挂入的waitqueue */
	struct list_head	q;
	wakeup_reason_e		wu_r;
	u32					timeout;		/* tick num */
} wait_task_t;

#define TASKSTACK_RANK			BUDDY_RANK_8K
#define TASKSTACK_SIZE			(PAGE_SIZE << TASKSTACK_RANK)

typedef enum _task_state {
    STATE_RUNNING = 0,
    STATE_WAIT,
    STATE_STOP,
    STATE_EXIT
} task_state_e;

//
#define		TASKFLAG_NEEDSCHED		(0x80000000)			/* need sched */
#define		TASKFLAG_KT				(0x00000001)			/* Kernel thread */

struct _task_stack;
typedef struct _task {
	const char *name;
	task_state_e    state;
	int     pid;
	u32		tskflag;

	/* task切换上下文 */
	u32     ctx_ip, ctx_sp;
	u32		pgd_pa;				/* task page directory phy addr */

	struct _task_stack *stack;

	/* 调度相关 */
	int		pri, pri_init;		/* 初始优先级 */
	u32		slice, slice_init, total_tick;
	struct list_head q;			/* link this task desc into q */
	wait_task_t		wait;		/* 链入wait_queue_t 当task挂起时用到 */
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

/* task运行栈空间 占据2页 */
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

/**/
asmlinkage void exit(int exit_code);

typedef int (*task_entry)(void *param);

/* ring 0 thread */
int kernel_thread(task_entry entry, const char *name, void *param);

void idleloop(void);

void thread_start(void);

u32 is_taskinit_done();

void wait_ms(u32 ms);			/* busy loop */

void systick(void);

int waitqueue_init(wait_queue_t *wq);

/* 唤醒 */
void wakeup_task(task_t *task, wakeup_reason_e wu_r);
/* 阻塞 */
wakeup_reason_e wait_task(task_t *task, wait_queue_t *wq, u32 timeout);


extern volatile u32	smp_nap;

#endif

