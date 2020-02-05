#include "coursework.h"
#include "linkedlist.h"
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// generates a predefined number of elements (e.g. NUMBER_OF_JOBS = 1000), and increments a shared counter
void producer(void *args);

// removes elements and decrements the shared counter for every element removed.
void consumer(void *args);

// visualize the element in buffer with '*'
void visualize();
int proID = 0;                          // ID for producers and consumers
int conID = 0;                          // (only one consumer and producer in this case though)
pthread_t conThread, proThread;         // create the single producer and single consumer threads as required
sem_t sSync;                           	// mutex semaphores
sem_t empty;                            // binary semaphore to delete producer
sem_t full;                             // counting semaphore to delete consumer
int bufferLength = 0;					// number of elements in the buffer
int totalProducerJob = 0;               // total number of jobs done by producer thread
int totalConsumerJob = 0;               // total number of jobs done by consumer thread
struct element * head = NULL;       	// head pointer pointing to the first node of the buffer
struct element * tail = NULL;       	// tail pointer pointing to the last node of the buffer


int main(){
    // initialize semaphores
    sem_init(&sSync, 0, 1);
    sem_init(&empty, 0, 0);             // binary semaphore whose value would not excceed 1
    sem_init(&full, 0, 0);

    // according to the description, there is only one producer and one consumer
    proID++;
    conID++;
    
    // create/run threads
    pthread_create(&proThread, NULL, (void *)producer, NULL);
    pthread_create(&conThread, NULL, (void *)consumer, NULL);
    
    // join producer and consumer threads with the main thread
    pthread_join(proThread, NULL);
    pthread_join(conThread, NULL);

    // producer thread and consumer thread finish
    proID--;
    conID--;

    // get the values of two threads
    int sSyncValue, emptyValue, fullValue;    
    sem_getvalue(&sSync, &sSyncValue);      // use pthread API to retrieve the value of a semaphore
    sem_getvalue(&empty, &emptyValue);
    sem_getvalue(&full, &fullValue);
    
    // print the values for the semaphores
    printf("sSync = %d, empty = %d, full = %d\n", sSyncValue, emptyValue, fullValue);
    return 0;
}


void producer(void *p){
    int temp = bufferLength;
    while (1){
        // if reach the target number of jobs, break the loop
        if (totalProducerJob == NUMBER_OF_JOBS)
        {
            break;
        }
        if (temp == MAX_BUFFER_SIZE)
        {
            sem_wait(&empty);           // go to sleep if the buffer is full
        }
        // ready to produce (the buffer is not full)
        sem_wait(&sSync);               
        char * element = (char*)malloc(sizeof(char));
        *element = '*';
        addLast(element, &head, &tail);
		bufferLength++;
        temp = bufferLength;
        totalProducerJob++;                 
        printf("Producer %d, Produced = %d, Consumed = %d: ", proID, totalProducerJob, totalConsumerJob);
        visualize();                    // display the exact number of elements currently in the buffer
        sem_post(&sSync);               
        sem_post(&full);                // wake up the consumer (element counter++)
    }
}


void consumer(void *p){
    int temp;
    while (1){      
        sem_wait(&full);                // check if the buffer is empty, if yes then go to sleep
        sem_wait(&sSync);               
        char * element = removeFirst(&head, &tail);
        free(element);
        bufferLength--;
        temp = bufferLength;
        totalConsumerJob++;                 
        printf("Consumer %d, Produced = %d, Consumed = %d: ", conID, totalProducerJob, totalConsumerJob);
        visualize();                    // display the exact number of elements currently in the buffer
        sem_post(&sSync);               
        
        // if the buffer is not full any more and the producer is not finished, wakeup the producer
        if (temp == MAX_BUFFER_SIZE - 1 && totalProducerJob != NUMBER_OF_JOBS)    
        {
            sem_post(&empty);           // wake up producer because there is space to produce
        }
        
        // if reach the target number of jobs, break the loop
        if (totalConsumerJob == NUMBER_OF_JOBS)
        {
            break;                      // reach the target number and break the loop           
        }
    }
}


void visualize(){
    // traverse the buffer(linked list) and print the data ("*")
    struct element * temp = head;
    while (temp != NULL)
    {
        printf("%c", *(char *)temp -> pData);
        temp = temp ->pNext;
    }
    printf("\n");
}
