#include "typedef.h"
#include "io.h"
#include "interrupt.h"
#include "desc.h"
#include "string.h"
#include "section.h"


void interrupt_init()
{
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
    outb(0xFF & (~0x04), PORT_8259A_MASTER_A1);
    outb(0xFF, PORT_8259A_SLAVER_A1);

    /* setup idt */
    memset((u8 *)idt_table, 0, 8 * 256);

    /* enable interrupt */
    _sti();
}
