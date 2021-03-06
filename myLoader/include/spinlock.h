#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include "typedef.h"
#include "public.h"
#include "interrupt.h"

typedef struct spinlock {
    volatile int lock;
    u32 flag;
} spinlock_t;

#define DEFINE_SPINLOCK(name)   spinlock_t name = {1};

static inline void spinlock_init(spinlock_t *lock)
{
	lock->lock = 1;
}

static inline void spin_lock(spinlock_t *lock)
{
    raw_local_irq_save(lock->flag);

    while (lock->lock == 0);
    lock->lock = 0;
    barrier();
}

static inline void spin_unlock(spinlock_t *lock)
{
    lock->lock = 1;
	
    raw_local_irq_restore(lock->flag);
}


#endif

