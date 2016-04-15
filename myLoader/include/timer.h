#ifndef _TIMER_H_
#define _TIMER_H_

#include "config.h"

/* input clock of 8253 */
#define PIT_TICK_RATE 	1193182ul

#define PIT_LATCH	((PIT_TICK_RATE + HZ/2) / HZ)


/*
 * How many MSB values do we want to see? We aim for
 * a maximum error rate of 500ppm (in practice the
 * real error is much smaller), but refuse to spend
 * more than 50ms on it.
 */
#define MAX_QUICK_PIT_MS 50
#define MAX_QUICK_PIT_ITERATIONS (MAX_QUICK_PIT_MS * PIT_TICK_RATE / 1000 / 256)


/* 0x40 0x41 0x42分别对应8253的三个timer, 0x43对应控制端口 */
#define PIT_TIMER0_IOADDR	0x40
#define PIT_TIMER1_IOADDR	0x41
#define PIT_TIMER2_IOADDR	0x42
#define	PIT_CTRL_IOADDR		0x43

/* 获取tsc */
static inline u64 get_tsc(void)
{
	u64		tsc;

	/* "=A" 是将edx:eax的值输出到64bit变量 */
	asm volatile("rdtsc" : "=A"(tsc));

	return tsc;
}

extern u64	cpu_freq_khz, sys_tick;

u64 get_cpu_freq();

void timer_init(void);

#endif

