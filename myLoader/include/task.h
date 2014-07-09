#ifndef _TASK_H_
#define _TASK_H_

#include "typedef.h"
#include "list.h"
#include "memory.h"

#define TASKSTACK_RANGE         BUDDY_RANK_8K
#define TASKSTACK_SIZE          (PAGE_SIZE << TASKSTACK_RANGE)

typedef enum _task_state {
    STATE_RUNNING = 0,
    STATE_WAIT,
    STATE_STOP
} task_state_e;

struct _task_stack;
typedef struct _task {
    task_state_e    state;
    int     pid;

    u32     ctx_ip, ctx_sp;

    struct list_head q;         /* link this task desc into q */
    
    struct _task_stack *stack;
} task_t;

asmlinkage task_t *__switch_to(task_t *prev, task_t *next);

typedef struct _task_stack {
    task_t task_space, *task;           /* now we just put the task at the head of stack */
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
void exit(int exit_code);

typedef int (*task_entry)(void *param);

/* ring 0 thread */
int kernel_thread(task_entry entry, void *param);

/* sleep
 * usec: micro second
 */
void sleep(int usec);

#endif

