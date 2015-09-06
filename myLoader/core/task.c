#include "typedef.h"
#include "task.h"
#include "list.h"
#include "section.h"
#include "command.h"
#include "module.h"
#include "spinlock.h"
#include "desc.h"
#include "interrupt.h"
#include "boot.h"
#include "memory.h"
#include "userbin.h"

/* running q */
DEFINE_SPINLOCK(task_rq_lock);
static LIST_HEAD(task_running);

/* stop q */
DEFINE_SPINLOCK(task_sq_lock);
static LIST_HEAD(task_stop);

/* exit q, those tasks has called exit(), but the body hasn't been reclaim yet */
DEFINE_SPINLOCK(task_eq_lock);
static LIST_HEAD(task_exit);

static memcache_t *task_cache;

static int freepid = 0;

/* we have only one tss */
tss_t	global_tss;

/* busy loop */
void wait_ms(u32 ms)
{
	u32 count;
	while (ms--)
	{
		for (count = 0; count < 4000; count++)
		{
			__asm__ __volatile__ (
				"nop			\n\t"
				:
				:
			);
		}
	}
}

u32 schedflag = 0;

__attribute__((regparm(3))) task_t *__switch_to(task_t *prev, task_t *next)
{
	schedflag = 0;

	/* switch TSS.sp0, ss0 is also the SYSDESC_DATA */
	ASSERT(global_tss.ss0 == SYSDESC_DATA);
	global_tss.sp0 = (u32)(next->stack) + TASKSTACK_SIZE;

	/* switch cr3 */
	global_tss.__cr3 = next->pgd_pa;
	setCR3(next->pgd_pa);
	
    return next;
}

void idleloop(void)
{
    while (1)
    {
        struct list_head *p, *pn;
        LIST_WALK_THROUTH_SAVE(p, &task_exit, pn)
        {
            spin_lock(&task_eq_lock);
            list_remove(p);
            spin_unlock(&task_eq_lock);
        
            task_t *t = GET_CONTAINER(p, task_t, q);

            page_free(t->stack);
        }
        schedule();
    }
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

    if (schedflag == 0)
    {
		schedflag = 1;
	}
	else
	{
		return;
	}

    if (next == NULL)
    {
        return;
    }

    if (prev == next)
    {
        return;
    }

    /* now the 'next' is at the head of the running q
       we have to move it ot the tail of the running q */
    spin_lock(&task_rq_lock);
    list_remove(&(next->q));
    list_add_tail(&(next->q), &task_running);
    spin_unlock(&task_rq_lock);

    /* context switch */
    switch_to(prev, next);
}

void tail_sched(void)
{
	// schedule();

	/* 由于该调度发生在返回用户态之前 如果发现此时依然是一个内核线程
	   则说明内核线程的主体函数返回了 此时直接报错并exit() */
#if 0
	if (current->tskflag & TASKFLAG_KT)
	{
	    printf("kernel thread exit.\n");
	    exit(0);		/* schedule */
	}
#endif
}

void exit(int exit_code)
{
    task_t *t = current;

    /* now we remove the task outof the running q */
    spin_lock(&task_rq_lock);
    list_remove(&(t->q));
    spin_unlock(&task_rq_lock);

    spin_lock(&task_eq_lock);
    list_add_head(&(t->q), &task_exit);
    spin_unlock(&task_eq_lock);

    /*  */
    schedule();
}

int kernel_thread(task_entry entry, void *param)
{
	ASSERT(entry);

    task_stack_t *newstack = (task_stack_t *)page_alloc(TASKSTACK_RANGE, MMAREA_NORMAL);
    task_t *newtask = (task_t *)memcache_alloc(task_cache);
    newstack->task = newtask;

    newtask->stack = newstack;
    newtask->state = STATE_RUNNING;
	newtask->tskflag = TASKFLAG_KT;
	newtask->pgd_pa = (u32)page_alloc(BUDDY_RANK_4K, MMAREA_NORMAL | PAF_ZERO);
	/* MM_NORMALMEM_RANGE部分目录项直接拷贝过来, 这部分的页目录是所有的task共享的 */
	memcpy((void *)newtask->pgd_pa, (void *)getCR3(), NORMALMEM_N_PDEENTRY * sizeof(pde_t));
	
    newtask->pid = freepid++;
    /* insert in to the running q, need to get lock first */
    spin_lock(&task_rq_lock);
    list_add_head(&(newtask->q), &task_running);
    spin_unlock(&task_rq_lock);

	/* prepare the stack */
	struct pt_regs *regs = (struct pt_regs *)((u32)newstack + TASKSTACK_SIZE) - 1;
	regs->xgs = (int)entry;			/* kt entry func */
	regs->orig_eax = (long)param;	/* kt entry func param */
	regs->ebp = (u32)newstack + TASKSTACK_SIZE;

    /**/
    newtask->ctx_ip = (u32)kt_after_switch;
    newtask->ctx_sp = (u32)regs;

    
    return newtask->pid;
}

int exec_test()
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

void sleep(int usec)
{

}

/* system tick int */
asmlinkage void lapictimer(void)
{
#if 1
	u32 flags;
	/* get the flag register */
	asm volatile("pushf ; pop %0"
				: "=rm" (flags)
				: /* no input */
				: "memory");
	/* test the IF */
	if (flags & (0x1 << 9))
	{
		conspc_printf(" int enable.");
	}
	else
	{
		conspc_printf("int disable.");
	}
#endif
}

/* this is the task0 init */
void __init sched_init(void)
{
	/* 初始化存放task描述符的cache */
	task_cache = memcache_create(sizeof(task_t), BUDDY_RANK_8K, "task desc");

    task_stack_t *stack = getcurstack();
    task_t *task0 = (task_t *)memcache_alloc(task_cache);
	stack->task = task0;

    /* we need to init current task 0 */
    task0->stack = stack;
    task0->state = STATE_RUNNING;
	task0->tskflag = TASKFLAG_KT;
    task0->pid = freepid++;
	/* read the CR3 */
	task0->pgd_pa = getCR3();
	
    /* insert in to the running q, need to get lock first */
    spin_lock(&task_rq_lock);
    list_add_tail(&(task0->q), &task_running);
    spin_unlock(&task_rq_lock);

#if 0
    /* init the tick timer */
    set_trap(CUSTOM_VECTOR_LAPICTIMER, lapictimer_entrance);
    *((u32 *)0xFEE00380) = 0x100000;   /* initial count */
    *((u32 *)0xFEE00320) = 0x20000 | CUSTOM_VECTOR_LAPICTIMER;
#endif

	/* init tss */
#if 1
	global_tss.ss0 = SYSDESC_DATA;
	global_tss.sp0 = (u32)stack;
	global_tss.__cr3 = task0->pgd_pa;
	global_tss.cs = SYSDESC_CODE;
	global_tss.es = SYSDESC_DATA;
	global_tss.ss = SYSDESC_DATA;
	global_tss.ds = SYSDESC_DATA;
	global_tss.fs = SYSDESC_DATA;

	set_tss(&global_tss, sizeof(global_tss));
	set_tr();
#endif
}

module_init(sched_init, 1);

#ifdef CONFIG_SMP

extern u32 initial_code, smpstart_addr, smp_apentry[0];
extern void loader_entry();
extern u32 smpap_initsp[16];

/*  */
volatile u32	smp_nap = 0;

volatile u32	smp_testflag = 0;

void testsmpentry(void)
{
	task_stack_t *stack = getcurstack();

	while (1)
	{
		if (smp_testflag == (u32)stack)
		{
			u32 *apicidreg = (u32 *)0xfee00020;
			printf("this is task stack:0x%#8x, apicid:%d\n"
				   "CR0: 0x%#8x, CR3: 0x%#8x\n",
				   smp_testflag, (*apicidreg) >> 24, getCR0(), getCR3());
			smp_testflag = 0;
		}
	}
}

static void __init smp_init(void)
{
	u32 smpinit_vect = (u32)jump2pe;
	ASSERT(initial_code == (u32)loader_entry);
	/* change the initial_code */
	printf("change the entry initial_code: 0x%#8x--->0x%#8x\n", initial_code, (u32)smp_apentry);
	initial_code = (u32)smp_apentry;

	smp_nap = 0;

	/* let's map the APIC register address */
	mmap(0xFEE00000, 0xFEE00000, PAGE_SIZE, 0);

	/* now we have to alloc some init task stack for each AP's */
	/* now let's begin AP's */
	*((volatile u32 *)0xFEE00310) = 0;
	/* 1. send INIT IPI, write the ICR with 0xC4500 */
	*((volatile u32 *)0xFEE00300) = 0xC4500;
	wait_ms(10);

	/* 2. send SIPI */
	*((volatile u32 *)0xFEE00300) = 0xC4600 | (smpinit_vect >> 12);
	wait_ms(200);

	/* 3. send SIPI */
	*((volatile u32 *)0xFEE00300) = 0xC4600 | (smpinit_vect >> 12);
	wait_ms(1000);

	/* wait for AP init, find out that how many AP's in system. */
	smpap_initsp[0] = (u32)page_alloc(BUDDY_RANK_8K, MMAREA_NORMAL);
	smpap_initsp[1] = (u32)page_alloc(BUDDY_RANK_8K, MMAREA_NORMAL);
	smpap_initsp[2] = (u32)page_alloc(BUDDY_RANK_8K, MMAREA_NORMAL);
	smpap_initsp[3] = (u32)page_alloc(BUDDY_RANK_8K, MMAREA_NORMAL);
	smpap_initsp[4] = (u32)page_alloc(BUDDY_RANK_8K, MMAREA_NORMAL);
	smpap_initsp[5] = (u32)page_alloc(BUDDY_RANK_8K, MMAREA_NORMAL);
	smpap_initsp[6] = (u32)page_alloc(BUDDY_RANK_8K, MMAREA_NORMAL);
	smpap_initsp[7] = (u32)page_alloc(BUDDY_RANK_8K, MMAREA_NORMAL);


	/* now let's all the AP runnnnnnnnnnnnnnnn */
	smpstart_addr = (u32)testsmpentry;
}
module_init(smp_init, 7);

static void cmd_schedop_opfunc(char *argv[], int argc, void *param)
{
    unsigned i;
    /* list all cmd and their info string */
    if (argc == 2)
    {
    	i = str2num(argv[1]);
		smp_testflag = i;
		printf("smp_testflag: 0x%#8x\n", smp_testflag);
    }
}

struct command cmd_schedop _SECTION_(.array.cmd) =
{
    .cmd_name   = "schedop",
    .info       = "Some of schedule operations.",
    .param      = NULL,
    .op_func    = cmd_schedop_opfunc,
};

#endif
