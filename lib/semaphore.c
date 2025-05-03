// User-level Semaphore

#include "inc/lib.h"

struct semaphore create_semaphore(char *semaphoreName, uint32 value)
{
	//TODO: [PROJECT'24.MS3 - #02] [2] USER-LEVEL SEMAPHORE - create_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
//	panic("create_semaphore is not implemented yet");
	struct semaphore sem ;
	struct __semdata *data;
	data= smalloc(semaphoreName , sizeof(struct __semdata) , 1 );
	sem.semdata=data;

//   if (sem == NULL) {
//		panic("Failed to allocate memory for semaphore!");
//	}
	sys_create_semaphore(semaphoreName,value ,&sem);
	return sem;
}
struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	//TODO: [PROJECT'24.MS3 - #03] [2] USER-LEVEL SEMAPHORE - get_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	struct semaphore sem;
	struct __semdata *data;
	data = sget(ownerEnvID , semaphoreName);
//	cprintf("get_sem : ownerID %d name:%s \n",ownerEnvID,semaphoreName);
	sem.semdata=data;


	return sem;
}

void wait_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #04] [2] USER-LEVEL SEMAPHORE - wait_semaphore
//	cprintf("wait_sem : sem at %x semCNT : %d , envID = %d\n",sem,sem.semdata->count,myEnv->env_id);
	sys_wait_semaphore(sem.semdata);
}

void signal_semaphore(struct semaphore sem)
{
//	cprintf("signal_sem : sem at %x semCNT : %d , envID = %d\n",sem,sem.semdata->count,myEnv->env_id);
	sys_signal_semaphore(sem.semdata);
}

int semaphore_count(struct semaphore sem)
{
	return sem.semdata->count;
}
