#include "typedef.h"
#include "debug.h"
#include "io.h"
#include "interrupt.h"
#include "desc.h"
#include "section.h"
#include "command.h"
#include "module.h"
#include "sem.h"

/* kbd controller port */
#define PORT_KBD_CMD                (0x60)
#define PORT_KBD_CONTROL_CMD        (0x64)
#define PORT_STATMASK_OUTPORT_FULL  (0x1)
#define PORT_STATMASK_INPORT_FULL   (0x2)

/* some kbd command, send these cmd by outb to 0x64 */
#define KBDCTRL_CMD_INPORT_R    (0xC0)
#define KBDCTRL_CMD_OUTPORT_R   (0xD0)
#define KBDCTRL_CMD_OUTPORT_W   (0xD1)

/* some special scan code */
#define KBD_SCANCODE_LEFTSHIFT  (0x2A)
#define KBD_SCANCODE_RIGHTSHIFT (0x36)

#define KBD_SCANCODE_CAP        (0x3A)

static u8 kbd_shiftup_map[] =
{
    /* 0x00 ~ 0x0F */
    0x00, 0x00,  '1',  '2',  '3',  '4',  '5',  '6',
     '7',  '8',  '9',  '0',  '-',  '=', '\b', '\t',
    /* 0x10 ~ 0x1F */
     'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
     'o',  'p',  '[',  ']', '\n', 0x00,  'a',  's',
    /* 0x20 ~ 0x2F */
     'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
    '\'',  '`', 0x00, '\\',  'z',  'x',  'c',  'v',
    /* 0x30 ~ 0x3F */
     'b',  'n',  'm',  ',',  '.',  '/', 0x00, 0x00,
    0x00,  ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x40 ~ 0x4F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x50 ~ 0x5F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x60 ~ 0x6F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x70 ~ 0x7F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static u8 kbd_shiftdown_map[] =
{
    /* 0x00 ~ 0x0F */
    0x00, 0x00,  '!',  '@',  '#',  '$',  '%',  '^',
     '&',  '*',  '(',  ')',  '_',  '+', '\b', '\t',
    /* 0x10 ~ 0x1F */
     'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
     'O',  'P',  '{',  '}', '\n', 0x00,  'A',  'S',
    /* 0x20 ~ 0x2F */
     'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',
     '"',  '~', 0x00,  '|',  'Z',  'X',  'C',  'V',
    /* 0x30 ~ 0x3F */
     'B',  'N',  'M',  '<',  '>',  '?', 0x00, 0x00,
    0x00,  ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x40 ~ 0x4F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x50 ~ 0x5F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x60 ~ 0x6F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x70 ~ 0x7F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* read until the 8042's buff get empty
   I just copy the code from the linux kernel :) */
#define MAX_8042_LOOPS	100000
#define MAX_8042_FF	32
static int empty_8042(void)
{
	u8 status;
	int loops = MAX_8042_LOOPS;
	int ffs   = MAX_8042_FF;

	while (loops--) {
		io_delay();

		status = inb(PORT_KBD_CONTROL_CMD);
		if (status == 0xff) {
			/* FF is a plausible, but very unlikely status */
			if (!--ffs)
				return -1; /* Assume no KBC present */
		}
		if (status & 1) {
			/* Read and discard input data */
			io_delay();
			(void)inb(PORT_KBD_CMD);
		} else if (!(status & 2)) {
			/* Buffers empty, finished! */
			return 0;
		}
	}

	return -1;
}


#define KBD_RAWINPUTBUFF_SIZE   (256)

/* this is the kbd raw input buff */
struct kbd_rawinput_buff{
    u8      buff[KBD_RAWINPUTBUFF_SIZE];
    u16      readpos, writepos;          // if readpos == writepos, it means no input data
}kbd_rawinput_buff;

static sem_t kbd_sem;

static void kbd_rawbuff_insert(u8 value)
{
	/* if buff full, just drop this new char */
	if (((kbd_rawinput_buff.writepos + 1) % KBD_RAWINPUTBUFF_SIZE) == kbd_rawinput_buff.readpos)
		return;

    kbd_rawinput_buff.buff[kbd_rawinput_buff.writepos++] = value;
    // modify the write pos
    kbd_rawinput_buff.writepos %= KBD_RAWINPUTBUFF_SIZE;

	sem_signal(&kbd_sem);
}

/* return -1 means no data in rawbuff */
int kbd_get_char()
{
	int ret = 0;

	if (kbd_rawinput_buff.writepos == kbd_rawinput_buff.readpos)
		sem_wait(&kbd_sem, TIMEOUT_INFINIT);

	ASSERT(kbd_rawinput_buff.writepos != kbd_rawinput_buff.readpos);
	ret = kbd_rawinput_buff.buff[kbd_rawinput_buff.readpos];

	// modify the read pos
	kbd_rawinput_buff.readpos += 1;
	kbd_rawinput_buff.readpos %= KBD_RAWINPUTBUFF_SIZE;

	return ret;
}

u8 kbdctrl_outport_r(void)
{
    outb(KBDCTRL_CMD_OUTPORT_R, PORT_KBD_CONTROL_CMD);
    io_delay();
    return inb(PORT_KBD_CMD);
}

void kbdctrl_outport_w(u8 cmd)
{
	empty_8042();

    outb(KBDCTRL_CMD_OUTPORT_W, PORT_KBD_CONTROL_CMD);

	empty_8042();

    outb(cmd, PORT_KBD_CMD);
}

void kbd_int(struct pt_regs *regs, void *param)
{
	/* need to disable and enable the kbd controller, copied from linux kernel */
	__asm__ __volatile__ (
		"inb	$0x61, %%al				\n\t"
		"jmp	1f						\n\t"
		"1:		jmp		2f				\n\t"
		"2:		orb		$0x80, %%al		\n\t"
		"jmp	3f						\n\t"
		"3:		jmp		4f				\n\t"
		"4:		outb	%%al, $0x61		\n\t"
		"jmp	5f						\n\t"
		"5:		jmp		6f				\n\t"
		"6:		andb	$0x7F, %%al		\n\t"
		"outb	%%al, $0x61				\n\t"
		:
		:
		:"memory"
	);

    static u8 volatile state_cap = 0, state_shift = 0;
    
    /* read the scancode from the kbd register */
    u8 scancode = inb(PORT_KBD_CMD);

    DEBUG("%#2x.", scancode);

    /* process the left and right 'SHIFT' */
    if (((scancode & (~0x80)) == KBD_SCANCODE_LEFTSHIFT) ||
        ((scancode & (~0x80)) == KBD_SCANCODE_RIGHTSHIFT))
    {
        /* key up */
        if (scancode & 0x80)
        {
            state_shift = 0;
        }
        else    /* key down */
        {
            state_shift = 1;
        }
        return;
    }

    /* process the 'CAP' */
    if ((scancode & (~0x80)) == KBD_SCANCODE_CAP)
    {
        if ((scancode & 0x80) == 0)
        {
            state_cap ^= 1;         // change the cap state
        }
        return;
    }

    if ((scancode & 0x80) == 0)
    {
        u8 ins_ch;
        if (state_shift)
        {
            ins_ch = kbd_shiftdown_map[scancode & (~0x80)];
        }
        else
        {
            ins_ch = kbd_shiftup_map[scancode & (~0x80)];
        }

        if (ins_ch == 0)
        {
            return;
        }

        if (state_cap)
        {
            if ((ins_ch >= 'a') && (ins_ch <= 'z'))
            {
                ins_ch += 'A' - 'a';
            }
            else if ((ins_ch) >= 'A' && (ins_ch <= 'Z'))
            {
                ins_ch -= 'A' - 'a';
            }
        }
        
        kbd_rawbuff_insert(ins_ch);
    }
}

static void cmd_reboot_opfunc(char *argv[], int argc, void *param)
{
    u8 outport_cmd = kbdctrl_outport_r();
    printk("Now rebooting the system...\n");
    kbdctrl_outport_w(outport_cmd & 0xFE);
}

struct command cmd_reboot _SECTION_(.array.cmd) =
{
    .cmd_name   = "reboot",
    .info       = "Reboot the system.",
    .op_func    = cmd_reboot_opfunc,
};

static void __init kbd_init(void)
{
    kbd_rawinput_buff.readpos = kbd_rawinput_buff.writepos = 0;

	sem_init(&(kbd_sem), 0);
	
	/* keyboard interrupt */
	interrup_register(X86_VECTOR_IRQ_21 - X86_VECTOR_IRQ_20,
					  kbd_int, NULL);
}

module_init(kbd_init, 2);
