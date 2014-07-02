#include "config.h"
#include "typedef.h"

void memcpy(void *dst, void *src, u16 len)
{
    while(len--)
    {
        *((u8 *)dst) = *((u8 *)src);
        dst++;
        src++;
    }
}

void memset(void *dst, u8 value, u16 len)
{
    while(len--)
    {
        *((u8 *)dst) = value;
        dst++;
    }
}

void memset_u16(u16 *dst, u16 value, u16 len)
{
    while(len--)
    {
        *dst++ = value;
    }
}

int memcmp(void *str1, void *str2, u32 len)
{
    u8 *buff1 = (u8 *)str1;
    u8 *buff2 = (u8 *)str2;

    while (len--)
    {
        if (*buff1 > *buff2)
        {
            return 1;
        }
        else if (*buff1 < *buff2)
        {
            return -1;
        }
        buff1++;
        buff2++;
    }

    return 0;
}

u16 strlen(char *str)
{
    u16 len = 0;
    while (*str++)
        len++;

    return len;
}

u32 str2num(char *str)
{
    u32 ret = 0, base = 10;
    
    if ((str[0] == '0') && ((str[1] == 'x') || (str[1] == 'X')))
    {
        base = 16;
        str += 2;
    }
    while (1)
    {
        char ch = *str++;
        if (ch == 0)
        {
            break;
        }
        ret *= base;
        
        if ((ch >= '0') && (ch <= '9'))
        {
            ret += (ch - '0');
        }
        else if ((ch >= 'a') && (ch <= 'f'))
        {
            ret += (ch - 'a' + 10);
        }
        else if ((ch >= 'A') && (ch <= 'F'))
        {
            ret += (ch - 'A' + 10);
        }
        else
            return 0;
    }

    return ret;
}

int isinteflag(char ch, u8 n_inteflag, char *inteflag)
{
    u32 count;
    for (count = 0; count < n_inteflag; count++)
    {
        if (ch == inteflag[count])
        {
            return 1;
        }
    }
    return 0;
}

int parse_str_by_inteflag(char *str, char *argv[], u8 n_inteflag, char *inteflag)
{
    int argc = 0, flag = 0;
    while (*str)
    {
        if (isinteflag(*str, n_inteflag, inteflag) == 0)
        {
            if (flag == 0)
            {
                argv[argc++] = str;
                flag = 1;
            }
        }
        else
        {
            if (flag == 1)
            {
                *str = 0;
                flag = 0;
            }
        }
        str++;
    }

    return argc;
}

