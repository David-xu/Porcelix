char old, *src = (char *)0x80000400;

char g_test = 0;

typedef struct {
	unsigned buf[7];
	unsigned esp;
} jmpbuf;

jmpbuf b;

volatile char test_func(char a);
int setjmp(jmpbuf *b);
void longjmp(jmpbuf *b);


void entry(void)
{
	char *buff = "user mode, hello.\n";

	__asm__ __volatile__ (
	"jmp entry_flag_end	\n\t"
	"entry_flag:		\n\t"
	".byte	0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x2e, 0x00	\n\t"
	"entry_flag_end:	\n\t"
	);

	*src = ' ';
	old = ' ';

	if (setjmp(&b))
	{
		__asm__ __volatile__ (
			"int $0x80	\n\t"
			:
			:"a"(0x12), "b"(*src)
		);
	}

	__asm__ __volatile__ (
		"int $0x80	\n\t"
		:
		:"a"(0x10), "b"(buff)
	);
	
	while (1)
	{
		if (old != *src)
		{
			old = *src;
			longjmp(&b);
#if 0
			__asm__ __volatile__ (
				"int $0x80	\n\t"
				:
				:"a"(0x12), "b"(*src)
			);
#endif
		}
	}
}

volatile char test_func(char a)
{
        return a + g_test;
}

int setjmp(jmpbuf *b)
{
	/* eax 是返回值不用存 ebp在当前栈中已经保存了 */
	__asm__ __volatile__ (
		"push	%%edi			\n\t"
		"push	%%esi			\n\t"
		"push	%%edx			\n\t"
		"push	%%ecx			\n\t"
		"push	%%ebx			\n\t"
		"movl	%%esp, 28(%%eax)\n\t"			/* b->esp = %%esp */
		"movl	%%eax, %%edi	\n\t"
		"movl	%%esp, %%esi	\n\t"
		"movl	$7, %%ecx		\n\t"
		"rep	movsl			\n\t"
		"add	$20, %%esp		\n\t"
		:
		:"a"(b)
		:"memory"
	);

	return 0;
}

void longjmp(jmpbuf *b)
{
	/* 模拟一个返回栈结构 实际上在b里面已经存在了 将b中底部的变量依次恢复到reg中 */
	__asm__ __volatile__ (
		"movl	%%eax, %%esi		\n\t"
		"movl	28(%%eax), %%edi	\n\t"
		"movl	%%edi, %%esp		\n\t"
		"movl	$7, %%ecx			\n\t"
		"rep	movsl				\n\t"
		"pop	%%ebx				\n\t"
		"pop	%%ecx				\n\t"
		"pop	%%edx				\n\t"
		"pop	%%esi				\n\t"
		"pop	%%edi				\n\t"
		"movl	$1, %%eax			\n\t"
		"pop	%%ebp				\n\t"
		"ret						\n\t"
		:
		:"a"(b)
	);
}

