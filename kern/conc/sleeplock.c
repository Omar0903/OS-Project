// Sleeping locks

#include "inc/types.h"
#include "inc/x86.h"
#include "inc/memlayout.h"
#include "inc/mmu.h"
#include "inc/environment_definitions.h"
#include "inc/assert.h"
#include "inc/string.h"
#include "sleeplock.h"
#include "channel.h"
#include "../cpu/cpu.h"
#include "../proc/user_environment.h"

void init_sleeplock(struct sleeplock *lk, char *name)
{
	init_channel(&(lk->chan), "sleep lock channel");
	init_spinlock(&(lk->lk), "lock of sleep lock");
	strcpy(lk->name, name);
	lk->locked = 0;
	lk->pid = 0;
}
int holding_sleeplock(struct sleeplock *lk)
{
	int r;
	acquire_spinlock(&(lk->lk));
	r = lk->locked && (lk->pid == get_cpu_proc()->env_id);
	release_spinlock(&(lk->lk));
	return r;
}
//==========================================================================

void acquire_sleeplock(struct sleeplock *lk)
{
	struct spinlock *lock=&(lk->lk);
	struct Channel *chann =&(lk->chan);
	acquire_spinlock(lock);
	while(lk->locked == 1)
	{
		sleep(chann,lock);
	}
	lk->locked=1;
	release_spinlock(lock);
}

void release_sleeplock(struct sleeplock *lk)
{
	struct spinlock *lock=&(lk->lk);
	acquire_spinlock(lock);
	if(queue_size(&(lk->chan).queue) > 0){
	wakeup_all(&(lk->chan));
	}

	lk->locked=0;
	release_spinlock(lock);
}





