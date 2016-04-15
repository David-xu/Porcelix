#include "sem.h"
#include "task.h"

/* send signal, wake up all waiting tasks */
int sem_signal(sem_t *sem)
{
	/* some tasks need to wake up */
	if (sem->ticket < 0)
	{
		spin_lock(&(sem->lock));

		/* 等待被唤醒的任务队列一定不为空 */
		ASSERT(!CHECK_LIST_EMPTY(&(sem->wq.wlh)));

		struct list_head *p = sem->wq.wlh.next;
		task_t *t;
		
		t = GET_CONTAINER(p, task_t, wait.q);

		/* 正常唤醒 */
		wakeup_task(t, WU_REASON_WAKEUP);

		spin_unlock(&(sem->lock));
	}

	if (sem->ticket < sem->max_ticket)
		sem->ticket += 1;

	return MLD_RET_OK;
}

/* timeout: ms */
int sem_wait(sem_t *sem, u32 timeout)
{
	ASSERT(current != NULL);

	sem->ticket -= 1;

	if (sem->ticket < 0)
	{
		/* block this task */
		if (wait_task(current, &(sem->wq), timeout) == WU_REASON_TIMEOUT)
		{
			
			return MLD_RET_TIMEOUT;
		}
	}

	return MLD_RET_OK;
}

int sem_init(sem_t *sem, u32 max_ticket)
{
	spinlock_init(&(sem->lock));
	sem->max_ticket = max_ticket;
	sem->ticket = 0;
	return waitqueue_init(&(sem->wq));
}


