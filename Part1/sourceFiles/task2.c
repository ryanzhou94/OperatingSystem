#include "coursework.h"
#include "linkedlist.h"
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// removes elements and decrements the shared counter for every element removed.
void consumer(void *args);

// generates a predefined number of elements (e.g. NUMBER_OF_JOBS = 1000), and increments a shared counter
void producer(void *args);

// displays the exact number of elements currently in the buffer every time the counter changes
void visualize();

int bufferLength = 0;                   // a global shared variable counting how many elements are in the unbounded buffer
int totalProduce = 0;                   // total number of jobs done by producer thread
int totalConsume = 0;                   // total number of jobs done by consumer thread
sem_t sSync;                            // mutex semaphores
sem_t sDelayConsumer;                   // empty buffer semaphores
pthread_t conThread, proThread;         // create the single producer and single consumer threads as required


int main(){
    // initialize semaphores
    sem_init(&sSync, 0, 1);             // sSync cannot be shared between processes
    sem_init(&sDelayConsumer, 0, 0);    // consumer cannot consume if there is no element

    // create/run threads
    pthread_create(&proThread, NULL, (void *)producer, NULL);
    pthread_create(&conThread, NULL, (void *)consumer, NULL);

    // join producer and consumer threads with the main thread
    pthread_join(proThread, NULL);
    pthread_join(conThread, NULL);

    // get the values of two threads
    int sSyncValue, sDelayConsumerValue;    
    sem_getvalue(&sSync, &sSyncValue);      // use pthread API to retrieve the value of a semaphore
    sem_getvalue(&sDelayConsumer, &sDelayConsumerValue);
    
    // print the values for the semaphores
    printf("sSync = %d, sDelayConsumer = %d\n", sSyncValue, sDelayConsumerValue);
    return 0;
}


void producer(void *p){
    while (1){
        sem_wait(&sSync);               // 1 -> 0 (--), check if producer is in cirtical section
        bufferLength++;                        // produce an element from buffer
        totalProduce++;                 // increase the total number
        printf("Producer, Produced = %d, Consumed = %d: ", totalProduce, totalConsume);
        visualize();                    // display the exact number of elements currently in the buffer
        if (bufferLength == 1)
        {
            sem_post(&sDelayConsumer);  
        }
        if (totalProduce == NUMBER_OF_JOBS)
        {
            break;                      // reach the target number and break the loop                 
        }
        sem_post(&sSync);               // 0 -> 1 (++) evoke the sSync semaphore
    }
    sem_post(&sSync);                   // 0 -> 1 (++) evoke the sSync semaphore
}


void consumer(void *p){
    sem_wait(&sDelayConsumer);          // 0 -> -1 (--), check if the buffer is empty, if yes, then go to sleep, otherwise continue
    int temp;                           // local variable used to save the value of elements
    while (1){
        sem_wait(&sSync);               // 1 -> 0 (--), check if producer is in cirtical section
        bufferLength--;                        // consume an element from buffer
        temp = bufferLength;                   
        totalConsume++;                 // increase the total number
        printf("Consumer, Produced = %d, Consumed = %d: ", totalProduce, totalConsume);
        visualize();                    // display the exact number of elements currently in the buffer
        sem_post(&sSync);               // 0 -> 1 (++) evoke the sSync semaphore
        // if the number of total consume has reached the maximum number of jobs, then terminate the thread without excuting sem_wait
        if (totalConsume == NUMBER_OF_JOBS)
        {
            break;                      // reach the target number and break the loop 
        }
        if (temp == 0)
        {
            sem_wait(&sDelayConsumer);  // 0 -> -1 (--) if there is nothing in the buffer, go to sleep
        }
    }
}


void visualize(){
    for (int i = 0; i < bufferLength; i++)
    {
        printf("*");
    }
    printf("\n");
}