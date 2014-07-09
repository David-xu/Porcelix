#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include "typedef.h"
#include "public.h"
#include "interrupt.h"

typedef struct spinlock {
    volatile int lock;
} spinlock_t;

#define DEFINE_SPINLOCK(name)   spinlock_t name = {1};

static inline void spin_lock(spinlock_t *lock)
{
    while (lock->lock == 0);
    lock->lock = 0;
    barrier();

    printf("lock\n");
}

static inline void spin_unlock(spinlock_t *lock)
{
    lock->lock = 1;

    printf("unlock\n");
}


#endif

