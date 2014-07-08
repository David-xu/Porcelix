#include "typedef.h"
#include "task.h"
#include "list.h"
#include "section.h"
#include "command.h"
#include "module.h"
#include "spinlock.h"

DEFINE_SPINLOCK(task_rq_lock);
static LIST_HEAD(task_running);

static int freepid = 0;

__attribute__((regparm(3))) task_t *__switch_to(task_t *prev, task_t *next)
{
    return next;
}

static void kernel_task_ret(void)
{
    /* nothing can be done, just exit() */
    printf("kernel thread exit.");
    exit(0);
}

static task_t *getnexttask(void)
{
    ASSERT(!(CHECK_LIST_EMPTY(&task_running)));

    return GET_CONTAINER(task_running.next, task_t, q);
}

void schedule(void)
{
    task_t *prev = current;
    task_t *next = getnexttask();

    if (prev == next)
    {
        return;
    }
    // printf("next pid:%d\n", next->pid);

    /* now the 'next' is at the head of the running q
       we have to move it ot the tail of the running q */
    spin_lock(&task_rq_lock);
    list_add_tail(&(prev->q), &task_running);
    list_remove(&(next->q));
    spin_unlock(&task_rq_lock);

    /* context switch */
    switch_to(prev, next);
}

void exit(int exit_code)
{

}

int kernel_thread(task_entry entry, void *param)
{
    task_stack_t *newstack = (task_stack_t *)page2phyaddr(page_alloc(TASKSTACK_RANGE));
    task_t *newtask;
    newstack->task = &(newstack->task_space);
    newtask = newstack->task;

    newtask->stack = newstack;
    newtask->state = STATE_RUNNING;
    newtask->pid = freepid++;
    /* insert in to the running q, need to get lock first */
    spin_lock(&task_rq_lock);
    list_add_head(&(newtask->q), &task_running);
    spin_unlock(&task_rq_lock);

    if (entry)
    {
        /* prepare the stack */
        u32 *sp = (u32 *)(newstack++) - 2;          /* take 2 u32 as the hole */
        *(--sp) = (u32)kernel_task_ret;             /* this is task_entry return addr */
        *(--sp) = (u32)param;
        /**/
        newtask->ctx_ip = (u32)entry;
        newtask->ctx_sp = (u32)sp;
    }
    else
    {

    }
    
    return newtask->pid;
}

static void __init sched_init(void)
{
    task_stack_t *stack = getcurstack();
    task_t *task0;

    /* we need to init current task 0 */
    task0 = stack->task = &(stack->task_space);
    task0->stack = stack;
    task0->state = STATE_RUNNING;
    task0->pid = freepid++;
    /* insert in to the running q, need to get lock first */
#if 0
    spin_lock(&task_rq_lock);
    list_add_tail(&(task0->q), &task_running);
    spin_unlock(&task_rq_lock);
#endif

    /* init the tick timer */
    
}

module_init(sched_init, 7);


