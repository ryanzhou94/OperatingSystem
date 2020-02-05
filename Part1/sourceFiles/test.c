#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>
#include <semaphore.h>

#include "coursework.h"
#include "linkedlist.h"

// binary semaphores
// the need of delay_producer and delay_consumer:
// 1) to guard against producer from producing element exceeding the buffer size
// 2) to guard against consumer from consume element when list is empty
sem_t sync, delay_producer, delay_consumer;

// counter variables
int totalProduced = 0,
    totalConsumed = 0;

// linkedlist structure
struct element *head = NULL,
               *tail = NULL;

int listLength = 0;

// function prototypes
void *producer();
void *consumer();

int main()
{
	// thread variables
	pthread_t threadProducer, threadConsumer;

	// semaphore setup
	sem_init(&sync, 0, 1);
	sem_init(&delay_producer, 0, 0);
	sem_init(&delay_consumer, 0, 0);

	// thread setup & execution
	pthread_create(&threadProducer, NULL, producer, NULL);
	pthread_create(&threadConsumer, NULL, consumer, NULL);

	pthread_join(threadProducer, NULL);
	pthread_join(threadConsumer, NULL);

    int sSyncValue, emptyValue, fullValue;    
    sem_getvalue(&sync, &sSyncValue);      // use pthread API to retrieve the value of a semaphore
    sem_getvalue(&delay_producer, &emptyValue);
    sem_getvalue(&delay_consumer, &fullValue);
    
    // print the values for the semaphores
    printf("sSync = %d, empty = %d, full = %d\n", sSyncValue, emptyValue, fullValue);

}

void *producer()
{

	while (1)
	{

		// enter critical section
		sem_wait(&sync);

		// produce a '*' and add to the linked list
		char *tempData = malloc(sizeof(char));
		*tempData = '*';
		addLast((void *)tempData, &head, &tail);

		// update counters
		totalProduced++;
		listLength++;
		int tempLength = listLength; // save in a temp local variable

		// traversing linked list to print all elements
		printf("Producer, Produced = %d, Consumed = %d: ", totalProduced, totalConsumed);
		struct element *tempNode = head;
		while (tempNode != NULL)
		{
			printf("%c", *(char *)(tempNode->pData));
			tempNode = tempNode->pNext;
		}
		puts("");

		sem_post(&sync);
		// exit critical section

		if (tempLength == 1)
			sem_post(&delay_consumer); // if the list is no longer empty, wake up the consumer
		if (totalProduced >= MAX_NUMBER_OF_JOBS)
			break; // exit once 100 elements produced
		if (tempLength == MAX_BUFFER_SIZE)
			sem_wait(&delay_producer); // if the list is full, then go to sleep
	}
}

void *consumer()
{

	sem_wait(&delay_consumer); // if the list is empty, then go to sleep

	while (1)
	{

		// enter critical section
		sem_wait(&sync);

		// remove a '*' from the linked list
		char *tempData = removeFirst(&head, &tail);
		free(tempData);

		// update counters
		totalConsumed++;
		listLength--;
		int tempLength = listLength; // save in a temp local variable

		// print out the list
		printf("Consumer, Produced = %d, Consumed = %d: ", totalProduced, totalConsumed);
		struct element *tempNode = head;
		while (tempNode != NULL)
		{
			printf("%c", *(char *)(tempNode->pData));
			tempNode = tempNode->pNext;
		}
		puts("");

		sem_post(&sync);
		// exit critical section

		if (tempLength == MAX_BUFFER_SIZE - 1)
			sem_post(&delay_producer); // if the list is no longer full, wake up the producer
		if (totalConsumed >= MAX_NUMBER_OF_JOBS)
			break; // exit once 100 elements consumed
		if (tempLength == 0)
			sem_wait(&delay_consumer); // if the list is empty, then go to sleep
	}
}
