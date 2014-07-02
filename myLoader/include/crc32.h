#ifndef _CRC32_H_
#define _CRC32_H_

/*
    CRC-32-IEEE 802.3
    x^{32} + x^{26} + x^{23} + x^{22} + x^{16} + x^{12} + x^{11} + x^{10} + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
    1 0000 0100 1100 0001 0001 1101 1011 0111
      0    4    c    1    1    d    b    7
    0x04C11DB7 or 0xEDB88320 (0xDB710641)
*/

#define POLY        (0x04C11DB7UL)

unsigned crc32_raw(void *buf, unsigned len);

unsigned crc32(void *buf, unsigned len);


#endif

