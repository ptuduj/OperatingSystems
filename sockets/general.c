#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include "general.h"
#include <sys/shm.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

void create_que (struct queue* que) {

    que->first = 0;
    que->last = 0;
    que->que_size = 0;

    for (int i =0; i<MAX_QUE_SIZE; i++){
        que->queue_array[i] = -1;
    }
}


int enqueue (struct queue* que, int elem){

    if (que->last == MAX_QUE_SIZE && que->que_size == MAX_QUE_SIZE)  return false;   // queue is full
    if (que->last == MAX_QUE_SIZE) que->last = 0;
    que->queue_array[que->last] = elem;
    que->last++;
    que->que_size++;
    return true;
}


int dequeue (struct queue* que){

    if (que->first != -1)  {
        int elem = que->queue_array[que->first];
        if (que->first != MAX_QUE_SIZE) que->first++;
        else que->first=0;
        que->que_size--;
        return elem;
    }
    return false;    // queue is empty
}

int get_que_size (struct queue* que){
    return que->que_size;
}

void queue_toString (struct queue* que){

    for (int i=0; i<que->que_size; i++){
        if (que->first+i < MAX_QUE_SIZE) printf("%d\t", que->queue_array[que->first+i]);
        else printf("%d\t", que->queue_array[i - (MAX_QUE_SIZE - que->first)]);
    }
    printf("\n");
}



