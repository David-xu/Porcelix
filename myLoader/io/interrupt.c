#include "typedef.h"
#include "public.h"
#include "io.h"
#include "interrupt.h"
#include "desc.h"
#include "ml_string.h"
#include "section.h"
#include "list.h"
#include "memory.h"

static struct list_head int_pubent_list[16];
static u8 pubent_init[16] = {0};


void interrupt_init()
{
    u8 intmask;

    for (intmask = 0; intmask < ARRAY_ELEMENT_SIZE(int_pubent_list); intmask++)
    {
        _list_init(&(int_pubent_list[intmask]));
    }

    /* disable interrupt */
    _cli();

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

void int10_public(void)
{
    interrupt_proc_t *int_proc;
    /* walk all interrupt 10 entrance */
    LIST_FOREACH_ELEMENT(int_proc, &(int_pubent_list[10]), int_list)
    {
        int_proc->intf(int_proc->param);
    }
}

int interrup_register(int irq, intproc_ent intf, void *param)
{
    interrupt_proc_t *int_proc;
    u8 intmask;

    if (irq != 10)
    {
        return -1;
    }

    /*  */
    if (!pubent_init[irq])
    {
        _cli();
        
        /* OCW1, set 8259A slaver INTMASK reg */
        intmask = inb(PORT_8259A_SLAVER_A1);    /* the INTMAST reg is the 0x21 and 0xA1 */
        outb(intmask & (~0x04), PORT_8259A_SLAVER_A1);
        set_idt(X86_VECTOR_IRQ_2A, int10_public_entrance);  /* this is the int 10 public entrance */
    
        pubent_init[irq] = 1;
        printf("int10_public_entrance register success.\n");

        _sti();
    }

    LIST_FOREACH_ELEMENT(int_proc, &(int_pubent_list[irq]), int_list)
    {
        if (int_proc->intf == intf)
        {
            return -2;
        }
    }
    /* alloc one interrupt_proc_t, now we just used one page
       we need to impl the slab facility */
    int_proc = (interrupt_proc_t *)page2phyaddr(page_alloc(BUDDY_RANK_4K));

    int_proc->intf = intf;
    int_proc->param = param;

    list_add_head(&(int_proc->int_list), &(int_pubent_list[irq]));

    return 0;
}


