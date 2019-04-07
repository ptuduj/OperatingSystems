#ifndef GENERAL_H
#define GENERAL_H

#define MAX_CLIENTS 15
#define MAX_QUE_SIZE 30

#define ADD 0
#define SUB 1
#define MUL 3
#define DIV 4
#define PING 5

struct queue {         // circular queue
    int first;
    int last;
    int que_size;                  // actual number of elements in a queue
    int queue_array[MAX_QUE_SIZE];
};

void create_que (struct queue* que);
int enqueue (struct queue* que, int elem);
int dequeue (struct queue* que);
int get_que_size (struct queue* que);
void queue_toString (struct queue* que);

struct Message { 
    int operator;
    int arg1;
    int arg2;
    int number;
};

struct Reply {
    int number;
    char name[40];
    int result;
};

#endif
