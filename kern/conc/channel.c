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
void sleep(struct Channel *chan, struct spinlock* lk)
{
//TODO: [PROJECT'24.MS1 - #10] [4] LOCKS - sleep
//Sleep function in progress... (TESTING IN.PROG)
    struct Env *current_process = get_cpu_proc();
    release_spinlock(lk);

    current_process->channel = chan;
    current_process->env_status = ENV_BLOCKED;

    LIST_INSERT_TAIL(&chan->queue, current_process);// Insert current process into the channel's queue

    acquire_spinlock(&ProcessQueues.qlock);

    sched();
    release_spinlock(&ProcessQueues.qlock);
    acquire_spinlock(lk);
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
//Wake-Up-One function in progress... (TESTING IN.PROG)
acquire_spinlock(&ProcessQueues.qlock);

        struct Env *last_process = LIST_LAST(&chan->queue);
        if (!LIST_EMPTY(&chan->queue)){
        LIST_REMOVE(&chan->queue, last_process);
        last_process->env_status = ENV_READY;
        sched_insert_ready(last_process);// Make it ready to run
        }
        else{
         release_spinlock(&ProcessQueues.qlock);
         return;
        }
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
//TODO: [PROJECT'24.MS1 - #12] [4] LOCKS - wakeup_all
//COMMENT THE FOLLOWING LINE BEFORE START CODING
//panic("wakeup_all is not implemented yet");

acquire_spinlock(&ProcessQueues.qlock);

       struct Env *last_process;
        while(!LIST_EMPTY(&chan->queue)){
        last_process = LIST_LAST(&chan->queue);
        LIST_REMOVE(&chan->queue, last_process);
        last_process->env_status = ENV_READY;
        sched_insert_ready(last_process);// Make it ready to run
        }
    release_spinlock(&ProcessQueues.qlock);
}

