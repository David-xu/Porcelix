#include "crc32.h"

unsigned crc32_raw(void *buf, unsigned len)
{
    unsigned reg = 0, bitoff = 0, carry = 0;
    unsigned char bitflag = 0x80, curbyte = 0;

    while ((bitoff >> 3) < len)
    {
        if ((bitoff & 0x7) == 0)
        {
            curbyte = ((unsigned char *)buf)[bitoff >> 3];
            bitflag = 0x80;
        }
        else
            bitflag >>= 1;
    
        carry = reg >> 31;
        reg <<= 1;
        reg |= ((curbyte & bitflag) ? 0x1 : 0x0);
        bitoff++;

        /* get one bit */
        if (carry)
        {
            reg ^= POLY;
        }
    }

    return reg;
}

unsigned crc32(void *buf, unsigned len)
{
    unsigned char r_buff[8] = {0};
    unsigned r = crc32_raw(buf, len);
    r_buff[0] = (unsigned char)(r >> 24);
    r_buff[1] = (unsigned char)(r >> 16);
    r_buff[2] = (unsigned char)(r >> 8);
    r_buff[3] = (unsigned char)(r >> 0);
    return crc32_raw(r_buff, sizeof(r_buff));
}

