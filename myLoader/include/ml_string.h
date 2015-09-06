#ifndef _ML_STRING_H_
#define _ML_STRING_H_

void memcpy(void *dst, void *src, u32 len);
void memset(void *dst, u8 value, u32 len);
void memset_u16(u16 *dst, u16 value, u16 len);
int strcmp(const char *str1, const char *str2);
int memcmp(void *str1, void *str2, u32 len);
u16 strlen(char *str);

u32 str2num(char *str);

int isinteflag(char ch, u8 n_inteflag, char *inteflag);
int parse_str_by_inteflag(char *str, char *argv[], u8 n_inteflag, char *inteflag);

#endif

