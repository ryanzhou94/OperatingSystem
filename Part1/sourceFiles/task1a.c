#include "coursework.h"
#include "linkedlist.h"
#include <stdlib.h>

// generates a number jobs (struct process) stored in a linked list
void generateLinkedList(struct element ** head, struct element ** tail);


int main(){
    struct element * head = NULL;       // head pointer pointing to the first node of the linked list
    struct element * tail = NULL;       // tail pointer pointing to the last node of the linked list
    generateLinkedList(&head, &tail);   // generate a linked list of processes
    double responseTimeSum = 0;         // Sum up total response time
    double turnAroundTimeSum = 0;       // Sum up total turn around time
    struct timeval startTime;           // the time of the beginning
    gettimeofday(&startTime, NULL);     // set the start time

    // start running
    while(head != NULL){
        // get the head process of the linked list
        struct process * temp = (struct process *)(head -> pData);  

        runNonPreemptiveJob(temp, &(temp->oTimeCreated), &(temp->oMostRecentTime));

        responseTimeSum += getDifferenceInMilliSeconds(startTime, temp -> oTimeCreated);
        turnAroundTimeSum += getDifferenceInMilliSeconds(startTime, temp -> oMostRecentTime);

        printf("Process Id = %d, Previous Burst Time = %d, New Burst Time = %d, Response Time = %ld, Turn Around Time = %ld\n", temp -> iProcessId, temp -> iInitialBurstTime, temp -> iRemainingBurstTime, getDifferenceInMilliSeconds(startTime, temp -> oTimeCreated), getDifferenceInMilliSeconds(startTime, temp -> oMostRecentTime));
        
        // remove the finished process from the linked list (FCFS)
        removeFirst(&head, &tail); 
        
        // free allocated memory
        free(temp);
        
        // make sure pointers point to NULL    
        temp = NULL;
    }

    printf("Average response time = %lf\n", responseTimeSum / NUMBER_OF_JOBS );
    printf("Average turn around time = %lf\n", turnAroundTimeSum / NUMBER_OF_JOBS );
    return 0;
}

void generateLinkedList(struct element ** head, struct element ** tail){
    // create NUMBER_OF_JOBS of processeds
    for(int i = 0; i < NUMBER_OF_JOBS; i++){
        
        // create a process with details
        struct process * temp = generateProcess();
        
        // add the element(process) into the linked list
        addLast(temp, head, tail);
    }
}