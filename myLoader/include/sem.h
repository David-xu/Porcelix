#ifndef _SEM_H_
#define _SEM_H_

#include "list.h"
#include "task.h"
#include "spinlock.h"

typedef struct _sem {
	spinlock_t		lock;

	wait_queue_t	wq;

	int		ticket, max_ticket;
} sem_t;

/**/
int sem_signal(sem_t *sem);
/* timeout: ms */
int sem_wait(sem_t *sem, u32 timeout);

int sem_init(sem_t *sem, u32 max_ticket);

#endif

