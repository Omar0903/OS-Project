/*
 * channel.c
 *
 *  Created on: Sep 22, 2024
 *      Author: HP
 */
#include "channel.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <inc/string.h>
#include <inc/disk.h>

//===============================
// 1) INITIALIZE THE CHANNEL:
//===============================
// initialize its lock & queue
void init_channel(struct Channel *chan, char *name)
{
	strcpy(chan->name, name);
	init_queue(&(chan->queue));
}

//===============================
// 2) SLEEP ON A GIVEN CHANNEL:
//===============================
// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// Ref: xv6-x86 OS code
void sleep(struct Channel *chan,struct spinlock *lk){
		acquire_spinlock(&(ProcessQueues.qlock));
		release_spinlock(lk);
			get_cpu_proc()->env_status =ENV_BLOCKED;
			enqueue(&(chan->queue),get_cpu_proc());
			sched();
			acquire_spinlock(lk);
			release_spinlock(&(ProcessQueues.qlock));
}

//==================================================
// 3) WAKEUP ONE BLOCKED PROCESS ON A GIVEN CHANNEL:
//==================================================
// Wake up ONE process sleeping on chan.
// The qlock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes
void wakeup_one(struct Channel *chan)
{
	//TODO: [PROJECT'24.MS1 - #11] [4] LOCKS - wakeup_one
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("wakeup_one is not implemented yet");
	//Your Code is Here...
    struct Env *env;

    // Acquire the lock for the process queues
    acquire_spinlock(&ProcessQueues.qlock);

    // Check if there are any blocked processes in the channel
    if (queue_size(&chan->queue) > 0) {
        // Dequeue the first process from the channel's queue
        env = dequeue(&chan->queue);

        // Change the environment status to ready
        env->env_status = ENV_READY;

        // Insert the environment back into the ready queue
        sched_insert_ready0(env);
    }

    // Release the lock after modifying the queue
    release_spinlock(&ProcessQueues.qlock);
}
//====================================================
// 4) WAKEUP ALL BLOCKED PROCESSES ON A GIVEN CHANNEL:
//====================================================
// Wake up all processes sleeping on chan.
// The queues lock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes

void wakeup_all(struct Channel *chan)
{
    struct Env *env;

    // Acquire the lock for the process queues
    acquire_spinlock(&ProcessQueues.qlock);

    // While there are blocked processes in the channel's queue
    while (queue_size(&chan->queue) > 0)
    {
        // Dequeue the first process from the channel's queue
        env = dequeue(&chan->queue);

        // Change the environment status to ready
        env->env_status = ENV_READY;

        // Insert the environment back into the ready queue
        sched_insert_ready0(env);
    }

    // Release the lock after modifying the queue
    release_spinlock(&ProcessQueues.qlock);
}
