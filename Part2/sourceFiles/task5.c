#include "coursework.h"
#include "linkedlist.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>

void producer(void *args);
void consumer(void *args);
void boost(void *args);

struct process * processJob(int iConsumerId, struct process * pProcess, struct timeval oStartTime, struct timeval oEndTime);
struct process * lowestPriority(void);
int preemptedFCFS (struct process * newProcess);
void setProcessEnter(struct process * pro);
void setProcessLeave(struct process * pro);
void sleepProducer(long int waitTime);

struct element * head[MAX_PRIORITY] = {NULL};
struct element * tail[MAX_PRIORITY] = {NULL};
struct process * runningProcess[NUMBER_OF_CONSUMERS] = {NULL};

sem_t sSync;             // sync the critical section
sem_t full, empty;       // sync the main buffer (all jobs), not excceeds the MAX_BUFFER_SIZE
sem_t delay_consumer;    // sync the total number of jobs consumed

int iTemsProduced = 0;   // cannot excceed NUMBER_OF_JOBS
int iTemsConsumed = 0;   // cannot excceed NUMBER_OF_JOBS
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
    pthread_t boostingPriority;

    // initialize id arrays
    int ProID = 0;
    int ConID[NUMBER_OF_CONSUMERS];
    for (int i = 0; i < NUMBER_OF_CONSUMERS; i++)
    {
        ConID[i] = i;
    }
    
    // create 1 producer, 3 consumers threads and a boosting thread
    pthread_create(&producerThread, NULL, (void *)producer, (void *)&ProID);
    for (int i = 0; i < NUMBER_OF_CONSUMERS; i++) {
        pthread_create(&consumerThread[i], NULL, (void *)consumer, (void *)&ConID[i]);
    }
    pthread_create(&boostingPriority, NULL, (void *)boost, NULL);

    // join 1 producer, 3 consumers threads and a boosting thread to main thread
    pthread_join(producerThread, NULL);
    for (int i = 0; i < NUMBER_OF_CONSUMERS; i++) {
        pthread_join(consumerThread[i], NULL);
    }
    pthread_join(boostingPriority, NULL);

    // get the value of semaphores
    // int sSyncValue, emptyValue, fullValue, delay_consumerValue;    
    // sem_getvalue(&sSync, &sSyncValue);     
    // sem_getvalue(&empty, &emptyValue);
    // sem_getvalue(&full, &fullValue);
    // sem_getvalue(&delay_consumer, &delay_consumerValue);
    // printf("sSync = %d, empty = %d, full = %d, delay_consumer = %d\n", sSyncValue, emptyValue, fullValue, delay_consumerValue);

    // print average time
    printf("Average response time = %lf\n", dAverageResponseTime / NUMBER_OF_JOBS );
    printf("Average turn around time = %lf\n", dAverageTurnAroundTime / NUMBER_OF_JOBS );

    return 0;
}


void producer(void *args){
    while (1) {
        if (iTemsProduced >= NUMBER_OF_JOBS) {
            break;
        } else {
            iTemsProduced++;
        }

        sem_wait(&empty);       // check if the buffer is full, if full then go to sleep
        sem_wait(&sSync);       // enter the critical section if the buffer is not full

        struct process * newProcess = generateProcess();

        // add the process to the tail of one queue based on its priority
        addLast(newProcess, &head[newProcess -> iPriority], &tail[newProcess -> iPriority]);
        
        // print information
        printf("Producer %d, Process Id = %d (%s), Priority = %d, Initial Burst Time = %d\n", *(int *)args, newProcess -> iProcessId, newProcess->iPriority < MAX_PRIORITY / 2 ? "FCFS" : "RR", newProcess -> iPriority, newProcess -> iInitialBurstTime);

        if (newProcess -> iPriority < MAX_PRIORITY / 2) {
            if (preemptedFCFS(newProcess) == 1)
            {
                printf(" New Process Id %d, New priority %d\n", newProcess -> iProcessId, newProcess -> iPriority);
            }
        }
        // sleepProducer(MAX_BURST_TIME);
        sem_post(&sSync);       // leave critical section
        //sleepProducer(MAX_BURST_TIME);
        sem_post(&full);        // wake up consumers
        sleepProducer(MAX_BURST_TIME / 5);
    }
}


void consumer(void *args){
    while (1) {
        // occupy the job to prevent other threads from taking the same job at the same time
        // only one consumer thread is allowed to enter the if-else statment
        sem_wait(&sSync);      
        if (iTemsConsumed >= NUMBER_OF_JOBS) {
            sem_post(&sSync);
            break;
        } else {
            iTemsConsumed++;
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
        setProcessEnter(tempPro);   // push the process into processing section
        sem_post(&sSync);
        
        struct timeval start, end;
        runJob(tempPro, &start, &end);

        sem_wait(&sSync);
        setProcessLeave(tempPro);   // pop out the process from process section
        if (processJob(*(int *)args, tempPro, start, end) != NULL) {
            // if the job is not finished, add back to the queue
            iTemsConsumed--;
            addLast(tempPro, &head[priority], &tail[priority]);
        }
        sem_post(&sSync);
    }
}

void boost(void *args){
    struct timeval currentTime;
    while (1) {
        sem_wait(&sSync);
        for (int priority = (MAX_PRIORITY / 2) + 1; priority < MAX_PRIORITY; priority++) {
            if (head[priority] != NULL) {
                struct process * pro = (struct process *)(head[priority] -> pData);
                gettimeofday(&currentTime, NULL);
                if ((pro -> iInitialBurstTime == pro -> iRemainingBurstTime && getDifferenceInMilliSeconds(pro -> oTimeCreated, currentTime) >= BOOST_INTERVAL) || 
                (pro -> iInitialBurstTime > pro -> iRemainingBurstTime && getDifferenceInMilliSeconds(pro -> oMostRecentTime, currentTime) >= BOOST_INTERVAL))
                {
                    removeFirst(&head[priority], &tail[priority]);
                    addFirst(pro, &head[MAX_PRIORITY / 2], &tail[MAX_PRIORITY / 2]);
                    printf("Boost priority: Process Id = %d, Priority = %d, New Priority = %d\n", pro -> iProcessId, pro -> iPriority, MAX_PRIORITY / 2);
                }
            }   
        }
        if (iTemsConsumed >= NUMBER_OF_JOBS) {
            sem_post(&sSync);
            break;
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

// preempt the job that is running but has lower priority if three thread are all running FCFS
int preemptedFCFS (struct process * newProcess){
    struct process * lowPriPro = lowestPriority();
    if (lowPriPro == NULL) {
        return 0;
    } else if ((lowPriPro -> iPriority) > (newProcess -> iPriority)) {
        preemptJob(lowPriPro);
        printf("Pre-empted Process Id = %d, Pre-empted Priority %d,", lowPriPro -> iProcessId, lowPriPro -> iPriority);
        return 1;
    }
}

// if three consumers are all running and there exists FCFS consumer(s), return the process with the lowest priority
// otherwise, return NULL
struct process * lowestPriority(void){
    struct process * lowestProcess = NULL;
    int priority = -1;
    for (int i = 0; i < NUMBER_OF_CONSUMERS; i++)
    {
        if (runningProcess[i] == NULL) {
            return NULL;
        } else if ((runningProcess[i] -> iPriority) > priority && (runningProcess[i] -> iPriority) < (MAX_PRIORITY / 2)) {
            priority = runningProcess[i] -> iPriority;
            lowestProcess = runningProcess[i];
        }
    }
    return lowestProcess;
}

// let the process to the running array
void setProcessEnter(struct process * pro){
    int isAdded = 1;
    while (isAdded)
    {
        for (int i = 0; i < NUMBER_OF_CONSUMERS; i++)
        {
            if (runningProcess[i] == NULL)
            {
                runningProcess[i] = pro;
                isAdded = 0;
                break;
            }
        }
    }
}

// let the process to leave the running array
void setProcessLeave(struct process * pro){
    for (int i = 0; i < NUMBER_OF_CONSUMERS; i++)
    {
        if (runningProcess[i] == pro)
        {
            runningProcess[i] = NULL;
            return;
        }
    }
}

// another version of sleep()
void sleepProducer(long int waitTime){
    long int iDifference = 0;
    struct timeval oCurrent;
    struct timeval oStartTime;
    gettimeofday(&oStartTime, NULL);
    do
    {
        gettimeofday(&oCurrent, NULL);
        iDifference = getDifferenceInMilliSeconds(oStartTime, oCurrent);
    } while (iDifference < waitTime);
}
