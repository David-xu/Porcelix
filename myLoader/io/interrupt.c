#include "typedef.h"
#include "public.h"
#include "io.h"
#include "interrupt.h"
#include "desc.h"
#include "ml_string.h"
#include "section.h"
#include "list.h"
#include "memory.h"
#include "module.h"

static struct list_head int_pubent_list[16];
static u8 pubent_init[16] = {0};

INTPROC_DEFINE(0);
INTPROC_DEFINE(1);
INTPROC_DEFINE(2);
INTPROC_DEFINE(3);
INTPROC_DEFINE(4);
INTPROC_DEFINE(5);
INTPROC_DEFINE(6);
INTPROC_DEFINE(7);
INTPROC_DEFINE(8);
INTPROC_DEFINE(9);
INTPROC_DEFINE(10);
INTPROC_DEFINE(11);
INTPROC_DEFINE(12);
INTPROC_DEFINE(13);
INTPROC_DEFINE(14);
INTPROC_DEFINE(15);

typedef void (*intproc)(void);
const static intproc intprocarray[16] = 
{
	[0] = intproc_0,
	[1] = intproc_1,
	[2] = intproc_2,
	[3] = intproc_3,
	[4] = intproc_4,
	[5] = intproc_5,
	[6] = intproc_6,
	[7] = intproc_7,
	[8] = intproc_8,
	[9] = intproc_9,
	[10] = intproc_10,
	[11] = intproc_11,
	[12] = intproc_12,
	[13] = intproc_13,
	[14] = intproc_14,
	[15] = intproc_15,
};

static memcache_t *intproc_cache = NULL;

static inline void sendeoi_master(void)
{
	/* send EOI to 8259A master */
	__asm__ __volatile__ (
		"outb	%%al, %1"
		:
		:"a"(0x20), "i"(PORT_8259A_MASTER_A0)
	);
}

static inline void sendeoi_slaver(void)
{
	/* send EOI to 8259A slaver */
	__asm__ __volatile__ (
		"outb	%%al, %1"
		:
		:"a"(0x20), "i"(PORT_8259A_SLAVER_A0)
	);

	sendeoi_master();
}

asmlinkage void intprocjmp_master(struct pt_regs *regs, u32 irq)
{
    interrupt_proc_t *int_proc;

	ASSERT((irq >= 0) && (irq < 7));

    /* walk all interrupt entrance */
    LIST_FOREACH_ELEMENT(int_proc, &(int_pubent_list[irq]), int_list)
    {
        int_proc->intf(regs, int_proc->param);
    }

	sendeoi_master();
}

asmlinkage void intprocjmp_slaver(struct pt_regs *regs, u32 irq)
{
    interrupt_proc_t *int_proc;

	ASSERT((irq >= 8) && (irq < 16));
	
    /* walk all interrupt entrance */
    LIST_FOREACH_ELEMENT(int_proc, &(int_pubent_list[irq]), int_list)
    {
        int_proc->intf(regs, int_proc->param);
    }

	sendeoi_slaver();
}

void _SECTION_(.init.text) interrupt_init()
{
    u8 intmask;

    for (intmask = 0; intmask < ARRAY_ELEMENT_SIZE(int_pubent_list); intmask++)
    {
        _list_init(&(int_pubent_list[intmask]));
    }

    /* setup the 8259A */
    /* ICW1 */
    outb(0x11, PORT_8259A_MASTER_A0);
    outb(0x11, PORT_8259A_SLAVER_A0);

    /* ICW2 */
    outb(0x20, PORT_8259A_MASTER_A1);
    outb(0x28, PORT_8259A_SLAVER_A1);

    /* ICW3 */
    outb(0x04, PORT_8259A_MASTER_A1);   /* master:8'b0000_0100 */
    outb(0x02, PORT_8259A_SLAVER_A1);   /* slaver: */

    /* ICW4 */
    outb(0x01, PORT_8259A_MASTER_A1);
    outb(0x01, PORT_8259A_SLAVER_A1);

    /* OCW1 */
    outb(0xFF & (~0x04), PORT_8259A_MASTER_A1); /* slaver chip interrupt enable */
    outb(0xFF, PORT_8259A_SLAVER_A1);

    /* setup idt */
    memset((u8 *)idt_table, 0, 8 * 256);

    /* enable interrupt */
    _sti();
}

int interrup_register(int irq, intproc_ent intf, void *param)
{
    interrupt_proc_t *int_proc;
    u8 intmask;

	if (intproc_cache == NULL)
	{
		intproc_cache = memcache_create(sizeof(interrupt_proc_t), BUDDY_RANK_4K, "interrupt_proc_t");
		ASSERT(intproc_cache != NULL);
	}

	/* the slaver to master pin should not used. */
	ASSERT((irq != 2) & (irq < 16));

    /*  */
    if (!pubent_init[irq])
    {
        _cli();

		if (irq < 8)
		{
			/* OCW1, set 8259A master INTMASK reg */
		    intmask = inb(PORT_8259A_MASTER_A1);
			outb(intmask & (~(0x01 << irq)), PORT_8259A_MASTER_A1);
		}
		else
		{
	        /* OCW1, set 8259A slaver INTMASK reg */
	        intmask = inb(PORT_8259A_SLAVER_A1);
	        outb(intmask & (~(0x01 << (irq - 8))), PORT_8259A_SLAVER_A1);
		}

        set_intgate(X86_VECTOR_IRQ_20 + irq, intprocarray[irq]);

        pubent_init[irq] = 1;
        printf("int %d register new proc success.\n", irq);

        _sti();
    }

    LIST_FOREACH_ELEMENT(int_proc, &(int_pubent_list[irq]), int_list)
    {
        if (int_proc->intf == intf)
        {
            return -2;
        }
    }
    /* alloc one interrupt_proc_t */
	int_proc = (interrupt_proc_t *)memcache_alloc(intproc_cache);

    int_proc->intf = intf;
    int_proc->param = param;

    list_add_head(&(int_proc->int_list), &(int_pubent_list[irq]));

    return 0;
}

EXPORT_SYMBOL(interrup_register);

void _SECTION_(.init.text) trap_init(void)
{
	set_intgate(X86_VECTOR_NMI, NMIfault_entry);
	set_intgate(X86_VECTOR_DF, DFfault_entry);
	set_intgate(X86_VECTOR_TS, TSSfault_entry);
	set_intgate(X86_VECTOR_NP, NPfault_entry);
	set_intgate(X86_VECTOR_SS, SSfault_entry);
	set_intgate(X86_VECTOR_GP, GPfault_entry);
	set_intgate(X86_VECTOR_PF, PFfault_entry);

	/* syscall entry */
	set_trap(SYSTEMCALL_VECTOR, syscall_entry);
}


