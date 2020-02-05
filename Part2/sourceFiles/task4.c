#include "coursework.h"
#include "linkedlist.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

void producer(void *args);

void consumer(void *args);

struct process * processJob(int iConsumerId, struct process * pProcess, struct timeval oStartTime, struct timeval oEndTime);


struct element * head[MAX_PRIORITY] = {NULL};
struct element * tail[MAX_PRIORITY] = {NULL};

sem_t sSync;             // sync the critical section
sem_t full, empty;       // sync the main buffer (all jobs), not excceeds the MAX_BUFFER_SIZE
sem_t delay_consumer;    // sync the total number of jobs consumed

int totalProduced = 0;   // cannot excceed NUMBER_OF_JOBS
int totalConsumed = 0;   // cannot excceed NUMBER_OF_JOBS

double dAverageResponseTime = 0, dAverageTurnAroundTime = 0;


int main(){
    // initialization semaphores
    sem_init(&sSync, 0, 1);
    sem_init(&empty, 0, MAX_BUFFER_SIZE);
    sem_init(&full, 0, 0);
    sem_init(&delay_consumer, 0, 1);
    
    // define threads
    pthread_t consumerThread[NUMBER_OF_CONSUMERS];
    pthread_t producerThread;

    // initialize id arrays
    int ProID = 0;
    int ConID[NUMBER_OF_CONSUMERS];
    for (int i = 0; i < NUMBER_OF_CONSUMERS; i++)
    {
        ConID[i] = i; 
    }
    
    // create 1 producer and 3 consumers threads
    pthread_create(&producerThread, NULL, (void *)producer, (void *)&ProID);
    for (int i = 0; i < NUMBER_OF_CONSUMERS; i++) {
        pthread_create(&consumerThread[i], NULL, (void *)consumer, (void *)&ConID[i]);
    }

    // join 1 producer and 3 consumers threads to main thread
    pthread_join(producerThread, NULL);
    for (int i = 0; i < NUMBER_OF_CONSUMERS; i++) {
        pthread_join(consumerThread[i], NULL);
    }

    // get the value of semaphores
    // int sSyncValue, emptyValue, fullValue, delay_consumerValue;    
    // sem_getvalue(&sSync, &sSyncValue);     
    // sem_getvalue(&empty, &emptyValue);
    // sem_getvalue(&full, &fullValue);
    // sem_getvalue(&delay_consumer, &delay_consumerValue);
    // printf("sSync = %d, empty = %d, full = %d, delay_consumer = %d\n", sSyncValue, emptyValue, fullValue, delay_consumerValue);

    // print average times
    printf("Average response time = %lf\n", dAverageResponseTime / NUMBER_OF_JOBS );
    printf("Average turn around time = %lf\n", dAverageTurnAroundTime / NUMBER_OF_JOBS );

    return 0;
}


void producer(void *args){
    while (1) {
        // only one producer in this task, no need to sync 'totalProduced'
        if (totalProduced >= NUMBER_OF_JOBS) {
            break;
        } else {
            totalProduced++;
        }

        sem_wait(&empty);       // check if the buffer is full, if full then go to sleep
        sem_wait(&sSync);       // enter the critical section if the buffer is not full

        struct process * newProcess = generateProcess();

        // add the process to the tail of one queue based on its priority
        addLast(newProcess, &head[newProcess -> iPriority], &tail[newProcess -> iPriority]);
        
        // print information
        printf("Producer %d, Process Id = %d (%s), Priority = %d, Initial Burst Time = %d\n", *(int *)args, newProcess -> iProcessId, newProcess->iPriority < MAX_PRIORITY / 2 ? "FCFS" : "RR", newProcess -> iPriority, newProcess -> iInitialBurstTime);

        sem_post(&sSync);       // leave critical section
        sem_post(&full);        // wake up consumers
    }
}



// args is the thread id
void consumer(void *args){
    while (1) {
        // occupy the job to prevent other threads from taking the same job at the same time
        // only one consumer thread is allowed to enter the if-else statment
        sem_wait(&sSync);      
        if (totalConsumed >= NUMBER_OF_JOBS) {
            sem_post(&sSync);
            break;
        } else {
            totalConsumed++;
            sem_post(&sSync);
        }

        sem_wait(&full);        // check if there is job in the buffer
        sem_wait(&sSync);       // enter critical section

        // find job with highest priority
        int priority = 0;
        while (head[priority] == NULL) {
            priority++;
        }
        
        struct process * tempPro = (struct process *)removeFirst(&head[priority], &tail[priority]);
        
        sem_post(&sSync);

        struct timeval start, end;
        runJob(tempPro, &start, &end);      // run in parallel

        sem_wait(&sSync);

        if (processJob(*(int *)args, tempPro, start, end) != NULL) {
            totalConsumed--;
            addLast(tempPro, &head[priority], &tail[priority]);
        }
        sem_post(&sSync);
    }
}

struct process * processJob(int iConsumerId, struct process * pProcess, struct timeval oStartTime, struct timeval oEndTime)
{
	int iResponseTime;
	int iTurnAroundTime;
	if(pProcess->iPreviousBurstTime == pProcess->iInitialBurstTime && pProcess->iRemainingBurstTime > 0)
	{
		iResponseTime = getDifferenceInMilliSeconds(pProcess->oTimeCreated, oStartTime);	
		dAverageResponseTime += iResponseTime;
		printf("Consumer %d, Process Id = %d (%s), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Response Time = %d\n", iConsumerId, pProcess->iProcessId, pProcess->iPriority < MAX_PRIORITY / 2	 ? "FCFS" : "RR",pProcess->iPriority, pProcess->iPreviousBurstTime, pProcess->iRemainingBurstTime, iResponseTime);
		sem_post(&full);
        return pProcess;
	} else if(pProcess->iPreviousBurstTime == pProcess->iInitialBurstTime && pProcess->iRemainingBurstTime == 0)
	{
		iResponseTime = getDifferenceInMilliSeconds(pProcess->oTimeCreated, oStartTime);	
		dAverageResponseTime += iResponseTime;
		iTurnAroundTime = getDifferenceInMilliSeconds(pProcess->oTimeCreated, oEndTime);
		dAverageTurnAroundTime += iTurnAroundTime;
		printf("Consumer %d, Process Id = %d (%s), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Response Time = %d, Turnaround Time = %d\n", iConsumerId, pProcess->iProcessId, pProcess->iPriority < MAX_PRIORITY / 2 ? "FCFS" : "RR", pProcess->iPriority, pProcess->iPreviousBurstTime, pProcess->iRemainingBurstTime, iResponseTime, iTurnAroundTime);
		free(pProcess);
        sem_post(&empty);
		return NULL;
	} else if(pProcess->iPreviousBurstTime != pProcess->iInitialBurstTime && pProcess->iRemainingBurstTime > 0)
	{
		printf("Consumer %d, Process Id = %d (%s), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d\n", iConsumerId, pProcess->iProcessId, pProcess->iPriority < MAX_PRIORITY / 2 ? "FCFS" : "RR", pProcess->iPriority, pProcess->iPreviousBurstTime, pProcess->iRemainingBurstTime);
		sem_post(&full);
        return pProcess;
	} else if(pProcess->iPreviousBurstTime != pProcess->iInitialBurstTime && pProcess->iRemainingBurstTime == 0)
	{
		iTurnAroundTime = getDifferenceInMilliSeconds(pProcess->oTimeCreated, oEndTime);
		dAverageTurnAroundTime += iTurnAroundTime;
		printf("Consumer %d, Process Id = %d (%s), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Turnaround Time = %d\n", iConsumerId, pProcess->iProcessId, pProcess->iPriority < MAX_PRIORITY / 2 ? "FCFS" : "RR", pProcess->iPriority, pProcess->iPreviousBurstTime, pProcess->iRemainingBurstTime, iTurnAroundTime);
		free(pProcess);
        sem_post(&empty);
		return NULL;
	}
}
