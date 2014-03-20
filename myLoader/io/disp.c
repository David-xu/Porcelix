#include "io.h"
#include "string.h"
#include "assert.h"

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

static void vga_console_write(struct console *con, u8 *str, u16 len)
{
    struct vga_param *param = con->param;
    u16 i, j, k, *srcpix, *dstpix;
    u8 ch;
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
            param->x_pos = (param->x_pos + VGA_TABLE_WIDTH - 1) & (~(VGA_TABLE_WIDTH - 1));

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

static u16 num_fmt(u32 num, u8 *buf, u8 basenum, u16 width, u8 prefix, u8 flag)
{
    u8 tempbuf[16], idx = sizeof(tempbuf), len = 0;

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

/*

    S_CHAR------->S_PRE
       ^            |------------
       |            |     |     |
       |            V     V     V
       |          S_DEC S_OCT S_HEX
       |            |     |     |
       |            V     V     V
       --------------------------
*/


/* let's keep it simple */
/* todo: */
int printf(u8 *fmt, ...)
{
    u32 *args = ((u32 *)&(fmt)) + 1;
    u16 width = 0, idx = 0;
    u8 flag = ' ', ch, textbuf[256];
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
                    idx += num_fmt((u32)(-i), &(textbuf[idx]), 10, width, '-', flag);
                else
                    idx += num_fmt((u32)i, &(textbuf[idx]), 10, width, 0, flag);

                state = S_CHAR;
            }
            else if ((ch == 'x') || (ch == 'X'))
            {
                u32 i = *args++;
                idx += num_fmt(i, &(textbuf[idx]), 16, width, 0, flag);

                state = S_CHAR;
            }
            else if (ch == 'c')
            {
                u32 i = *args++;
                textbuf[idx++] = (u8)i;

                state = S_CHAR;
            }
            else if (ch == 's')
            {
                u8 chtmp, *src = (u8 *)(*args++);
                while (chtmp = *src++)
                {
                    textbuf[idx++] = chtmp;
                }

                state = S_CHAR;
            }
            else if (ch == '%')
            {
                textbuf[idx++] = '%';
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
                textbuf[idx++] = ch;
            }
        }
    }
    textbuf[idx++] = '\0';

    vga_console.write(&vga_console, textbuf, idx);
}

int sprintf(u8 *buf, u8 *fmt, ...)
{
    u32 *args = ((u32 *)&(fmt)) + 1;
    u16 width = 0, idx = 0;
    u8 flag = ' ', ch;
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
                u8 chtmp, *src = (u8 *)(*args++);
                while (chtmp = *src++)
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
}

/* dump stack */
void dump_stack(void)
{
    // printf("");
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

