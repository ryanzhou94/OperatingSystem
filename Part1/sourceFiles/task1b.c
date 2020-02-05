#include "coursework.h"
#include "linkedlist.h"
#include <stdlib.h>

// generates a number jobs (struct process) stored in a linked list
void generateLinkedList(struct element ** head, struct element ** tail);

// print processes' details
void printProcessLinkedList(struct element * head);


int main(){
    struct element * head = NULL;       // head pointer pointing to the first node of the linked list
    struct element * tail = NULL;       // tail pointer pointing to the last node of the linked list
    generateLinkedList(&head, &tail);   // generate a linked list of processes
    printProcessLinkedList(head);       // print out details about the linked list / ready queue
    double responseTimeSum = 0;         // Sum up total response time
    double turnAroundTimeSum = 0;       // Sum up total turn around time
    struct timeval startTime;           // the time of the beginning
    gettimeofday(&startTime, NULL);     // set the time
    struct element * node = head; 

    // start running from the head of the linked list
    while (node != NULL)
    {
        // the head process of the linked list
        struct process * temp = (struct process *)(node -> pData);  

        runPreemptiveJob(temp, &(temp->oTimeCreated), &(temp->oMostRecentTime));

        printf("Process Id = %d, Previous Burst Time = %d, Remaining Burst Time = %d", temp -> iProcessId, temp -> iPreviousBurstTime, temp -> iRemainingBurstTime);
        
        if (temp -> iPreviousBurstTime == temp -> iInitialBurstTime)    // the first running for this process
        {
            responseTimeSum += getDifferenceInMilliSeconds(startTime, temp -> oTimeCreated);   
            printf(", Response Time = %ld", getDifferenceInMilliSeconds(startTime, temp -> oTimeCreated));
        }
        
        if (temp -> iRemainingBurstTime == 0)   // the last running for this process
        {
            turnAroundTimeSum += getDifferenceInMilliSeconds(startTime, temp -> oMostRecentTime);
            printf(", Turnaround Time = %ld", getDifferenceInMilliSeconds(startTime, temp -> oMostRecentTime));
            free(temp);                         // free memory of process (not the node)
        } else {
            addLast(temp, &head, &tail);
        }
        printf("\n");

        node = node -> pNext;   
        removeFirst(&head, &tail);              // remove (free) the node (not the process)
    }

    printf("Average response time = %lf\n", responseTimeSum / NUMBER_OF_JOBS);
    printf("Average turn around time = %lf\n", turnAroundTimeSum / NUMBER_OF_JOBS);
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

void printProcessLinkedList(struct element * head){
    printf("PROCESS LIST:\n");
    struct element * tempNode = head;
    while (tempNode != NULL)
    {
        struct process * tempPro = (struct process *)tempNode ->pData;
        printf("\tProcess Id = %d, Priority = %d, Initial Burst Time = %d, Remaining Burst Time = %d\n", tempPro -> iProcessId, tempPro -> iPriority, tempPro -> iInitialBurstTime, tempPro -> iRemainingBurstTime);
        tempNode = tempNode -> pNext;
    }
    printf("END\n\n");
}
