#include "public.h"
#include "io.h"
#include "ml_string.h"
#include "assert.h"
#include "module.h"
#include "task.h"

#define VGA_RSELECT         (0x03D4)
#define VGA_RDATA           (0x03D5)
#define VGA_MODE            (0x03D8)
#define VGA_COPANEL         (0x03D9)

#define VGA_RAMBASE         (0xb8000)

#define VGA_TABLE_WIDTH     (0x08)

struct vga_param{
    u16 x_pos, y_pos, x_max, y_max;

    void (*screen_clear)(struct vga_param *param);
    void (*set_cursal)(struct vga_param *param);
};

static void vga_bks(struct console *con)
{
    struct vga_param *param = con->param;
    u16 *dstpix;
    if (param->x_pos == 0)
    {
        if (param->y_pos == 0)
        {

            return;
        }
        param->x_pos = param->x_max - 1;
        param->y_pos--;
    }
    else
    {
        param->x_pos--;
    }

    dstpix = (u16 *)(VGA_RAMBASE + ((param->x_max * param->y_pos + param->x_pos) << 1));
    *dstpix = 0x720;

    param->set_cursal(param);
}

static void vga_set_cursal(struct vga_param *param)
{
    // set the cursal r14 r15
    u16 cur_cursal = param->x_max * param->y_pos + param->x_pos;
    outb(14, VGA_RSELECT);
    outb((u8)(cur_cursal >> 8), VGA_RDATA);
    outb(15, VGA_RSELECT);
    outb((u8)(cur_cursal >> 0), VGA_RDATA);
}

static void vga_screen_clear(struct vga_param *param)
{
    memset_u16((u16 *)VGA_RAMBASE, 0x720, (param->x_max * param->y_max));

    param->x_pos = 0;
    param->y_pos = 0;

    // set the cursal r14 r15
    param->set_cursal(param);
}

static void vga_console_write(struct console *con, char *str, u16 len)
{
    struct vga_param *param = con->param;
    u16 i, j, k, *srcpix, *dstpix;
    char ch;
    /*  */
    while (((ch = *str++) != '\0') && (len-- != 0))
    {
        /*  */
        if ('\n' == ch)
        {
            param->x_pos = 0;
            (param->y_pos)++;
        }
        else if ('\b' == ch)
        {
            vga_bks(con);
        }
        else if ('\t' == ch)
        {
            param->x_pos = (param->x_pos + VGA_TABLE_WIDTH) & (~(VGA_TABLE_WIDTH - 1));

            if (param->x_pos >= param->x_max)
            {
                (param->y_pos)++;
                param->x_pos = 0;
            }
        }
        else
        {
            dstpix = (u16 *)(VGA_RAMBASE + ((param->x_max * param->y_pos + param->x_pos) << 1));

            *dstpix = ((0x7 << 8) | (unsigned short)ch);

            (param->x_pos)++;

            if (param->x_pos == param->x_max)
            {
                (param->y_pos)++;
                param->x_pos = 0;
            }
        }

        // scroll down 1 line
        if (param->y_pos == param->y_max)
        {
            // move the line j to line k
            for (j = 1, k = 0; j < param->y_max; j++, k++)
            {
                for (i = 0; i < param->x_max; i++)
                {
                    srcpix = (u16 *)(VGA_RAMBASE + ((param->x_max * j + i) << 1));
                    dstpix = (u16 *)(VGA_RAMBASE + ((param->x_max * k + i) << 1));
                    *dstpix = *srcpix;
                }
            }
            (param->y_pos)--;

            /* clear the last line */
            dstpix = (u16 *)(VGA_RAMBASE + ((param->x_max * k) << 1));
			for (i = 0; i < param->x_max; i++)
            {
                *dstpix++ = 0x720;
            }         
        }

        param->set_cursal(param);
    }

	if (con->spcdisp_swch)
	{
		j = ARRAY_ELEMENT_SIZE(con->spcbuff);
		dstpix = (u16 *)(VGA_RAMBASE + ((param->x_max - j) << 1));
		for (i = 0; i < j; i++)
		{
			*dstpix = ((0x7 << 8) | (unsigned short)(con->spcbuff[i]));
			dstpix++;
		}
	}
}

static void vga_console_spcdisp(struct console *con, char *buf, u16 len, u8 swch)
{
	u16 i, j, *dstpix, spcbuffsize = ARRAY_ELEMENT_SIZE(con->spcbuff);
	if (swch == 0)
	{
		con->spcdisp_swch = 0;
	}
	else if (swch != 1)
	{
		return;
	}

	if (len > spcbuffsize)
	{
		len = spcbuffsize;
	}
	memset(con->spcbuff, 0, spcbuffsize);
	memcpy(con->spcbuff + (spcbuffsize - len), buf, len);
	con->spcdisp_swch = 1;
	if (con->spcdisp_swch)
	{
		j = ARRAY_ELEMENT_SIZE(con->spcbuff);
		dstpix = (u16 *)(VGA_RAMBASE + ((((struct vga_param *)(con->param))->x_max - j) << 1));
		for (i = 0; i < j; i++)
		{
			*dstpix = ((0x7 << 8) | (unsigned short)(con->spcbuff[i]));
			dstpix++;
		}
	}
}


static void vga_console_init(struct console *con)
{
    struct vga_param *param = con->param;
    u16 cur_pos;
    /* read r14 r15 */
    outb(14, VGA_RSELECT);
    cur_pos = inb(VGA_RDATA);
    cur_pos <<= 8;
    outb(15, VGA_RSELECT);
    cur_pos |= inb(VGA_RDATA);

    param->x_max = 80;
    param->y_max = 25;

    /* set the cursal */
    param->x_pos = cur_pos % param->x_max;
    param->y_pos = cur_pos / param->x_max;

    param->screen_clear = vga_screen_clear;
    param->set_cursal = vga_set_cursal;
}

struct vga_param vga_param_s;
struct console vga_console =
{
    .name = "vga",
    .init = vga_console_init,
    .write = vga_console_write,
    .spcdisp = vga_console_spcdisp,
    .param = &vga_param_s,
};

void test_bks()
{
    vga_bks(&vga_console);
}

void disp_init()
{    
    vga_console.init(&vga_console);

    ((struct vga_param *)(vga_console.param))->screen_clear(vga_console.param);

    printf("********************************************************************************");
    printf("*                                                                              *");
    printf("*              This is myLoader, all rights reserved by David.                 *");
    printf("*                                                                              *");
    printf("********************************************************************************\n");
    printf("System Begin...\n");
}


/******************************************************************************/
/*  this is the print functions                                               */

/******************************************************************************/


u8 n2c(u8 num)
{
    if ((num >= 0) && (num <= 9))
    {
        return (num + '0');
    }
    else
    {
        return (num - 10 + 'A');
    }
}

/* if width == 0, then output the whole texts */
/* basenum: 10 or 16 or 8 */

static u16 num_fmt(u32 num, char *buf, u8 basenum, u16 width, char prefix, char flag)
{
    char tempbuf[16];
    u8 idx = sizeof(tempbuf), len = 0;

    if (0 == num)
    {
        tempbuf[--idx] = '0';
        len++;
    }
    else
    {
        while (num)
        {
            tempbuf[--idx] = n2c((u8)(num % basenum));
            num /= basenum;
            len++;
        }
    }

    if (prefix)
    {
        tempbuf[--idx] = prefix;
        len++;
    }

    if (len >= width)
    {
        memcpy(buf, &tempbuf[idx], len);
        return len;
    }

    if ((flag == '#') || (flag == ' '))
    {
        if (flag == '#')
            memset(buf, '0', width - len);
        else
            memset(buf, ' ', width - len);
        memcpy(buf + width - len, &tempbuf[idx], len);
    }
    else if (flag == '+')
    {
        /* left align */
        memcpy(buf, &tempbuf[idx], len);
        memset(buf + len, ' ', width - len);
    }    
    else
    {
        ASSERT(0);
    }
    
    
    return width;
}


/*
 *  %[flags][width][.perc] [F|N|h|l]type
 */
enum PRINTF_STATE
{
    S_CHAR,
    S_PRE_FLAG,
    S_PRE_WIDTH,
    S_PRE_PERC,
    S_PRE_F_N_H_L,
    S_DEC,
    S_OCT,
    S_HEX,
};


/* let's keep it simple */
int printf(char *fmt, ...)
{
	u32 *args = ((u32 *)&(fmt)) + 1;
	char textbuf[512];
	int len = vsprintf(textbuf, fmt, args);

	vga_console.write(&vga_console, textbuf, len);

	return 0;
}
EXPORT_SYMBOL(printf);


int conspc_printf(char *fmt, ...)
{
    u32 *args = ((u32 *)&(fmt)) + 1;
    char textbuf[ARRAY_ELEMENT_SIZE(vga_console.spcbuff)];
	int len;

	memset(textbuf, 0, sizeof(textbuf));
	len = vsprintf(textbuf, fmt, args);

	vga_console.spcdisp(&vga_console, textbuf, len, 1);

	return 0;
}

void conspc_off(void)
{
	vga_console.spcdisp(&vga_console, NULL, 0, 0);
}

int sprintf(char *buf, char *fmt, ...)
{
	u32 *args = ((u32 *)&(fmt)) + 1;

	return vsprintf(buf, fmt, args);
}
EXPORT_SYMBOL(sprintf);

/*

    S_CHAR------->S_PRE
       ^            |------------
       |            |     |     |
       |            V     V     V
       |          S_DEC S_OCT S_HEX
       |            |     |     |
       |            V     V     V
       --------------------------
 return text len
*/
int vsprintf(char *buf, char *fmt, u32 *args)
{
    u16 width = 0, idx = 0;
    char flag = ' ', ch;
    enum PRINTF_STATE state = S_CHAR;
    
    while ((ch = *fmt++) != '\0')
    {   
        if ((state == S_PRE_FLAG) ||
            (state == S_PRE_WIDTH) ||
            (state == S_PRE_PERC) ||
            (state == S_PRE_F_N_H_L))
        {
            if ((ch == '#') ||
                (ch == '+') ||
                (ch == '-') ||
                (ch == ' '))
            {            
                /* (state == S_PRE_FLAG) || (state == S_PRE_WIDTH) */
                ASSERT(state == S_PRE_FLAG);
                
                flag = ch;

                /* change to S_PRE_WIDTH */
                state = S_PRE_WIDTH;
            }
            else if ((ch >= '0') && (ch <= '9'))
            {
                /* (state == S_PRE_FLAG) || (state == S_PRE_WIDTH) */
                ASSERT((state == S_PRE_FLAG) || (state == S_PRE_WIDTH));

                width *= 10;
                width += (ch - '0');
            }
            else if (ch == '.')
            {
                /* (state == S_PRE_FLAG) || (state == S_PRE_WIDTH) */
                ASSERT((state == S_PRE_FLAG) || (state == S_PRE_WIDTH));



                /* change to S_PRE_PERC */
                state = S_PRE_PERC;
            }
            else if ((ch == 'd') || (ch == 'i'))
            {
                int i = *args++;

                if (i < 0)
                    idx += num_fmt((u32)(-i), &(buf[idx]), 10, width, '-', flag);
                else
                    idx += num_fmt((u32)i, &(buf[idx]), 10, width, 0, flag);

                state = S_CHAR;
            }
            else if ((ch == 'x') || (ch == 'X'))
            {
                u32 i = *args++;
                idx += num_fmt(i, &(buf[idx]), 16, width, 0, flag);

                state = S_CHAR;
            }
            else if (ch == 'c')
            {
                u32 i = *args++;
                buf[idx++] = (u8)i;

                state = S_CHAR;
            }
            else if (ch == 's')
            {
                char chtmp, *src = (char *)(*args++);
                while ((chtmp = *src++) != 0)
                {
                    buf[idx++] = chtmp;
                }

                state = S_CHAR;
            }
            else if (ch == '%')
            {
                buf[idx++] = '%';
                state = S_CHAR;
            }
            else
            {
            
   ASSERT(0);
            }
        }
        else    /* char */
        {
            if (ch == '%')
            {
                state = S_PRE_FLAG;
                width = 0;
                flag = ' ';
            }
            else    /* S_CHAR */
            {
                buf[idx++] = ch;
            }
        }
    }
    buf[idx++] = '\0';

    return idx;
}


/* dump stack */
void dump_stack(u32 ebp)
{
	u32 *cur_ebp = (u32 *)ebp;
	u32 cur_retaddr, funcbase;
	u32 top_ebp = (u32)(current->stack) + TASKSTACK_SIZE;
	char *funcname;

	printf("call stack:\n");

	while (cur_ebp != (u32 *)top_ebp)
	{
		cur_retaddr = *(cur_ebp + 1);

		funcname = allsym_resolvaddr(cur_retaddr, &funcbase);
		printf("ebp: 0x%#8x   eip: 0x%#8x   %s()  +  0x%#4x\n", (u32)cur_ebp, cur_retaddr, &(funcname[1]), cur_retaddr - funcbase);

		/* pop one stack from */
		cur_ebp = (u32 *)(*cur_ebp);
	}
}


/* block == 4, 2, 1 */
#define DUMPRAM_LINESIZE        (16)
void dump_ram(void *addr, u16 len)
{
    u16 count;
    for (count = 0; count < len; count++)

    {
        if ((count % DUMPRAM_LINESIZE) == 0)
        {
            printf("0x%#8x: ", (u32)addr);
        }

        printf("%#2x ", *((u8 *)addr));
        addr++;

        if ((count % DUMPRAM_LINESIZE) == (DUMPRAM_LINESIZE / 2 - 1))
        {
            printf("    ");
        }
        
        if ((count % DUMPRAM_LINESIZE) == (DUMPRAM_LINESIZE - 1))
        {
            printf("\n");
        }
    }
    if ((count % DUMPRAM_LINESIZE) != 0)
    {
        printf("\n");
    }
}

