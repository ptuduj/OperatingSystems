#ifndef GENERAL_H_INCLUDED
#define GENERAL_H_INCLUDED

#define true 1
#define false 0
#define MAX_QUE_SIZE 10
#define BARBER_ID 'B'


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
void print_time1(char * input, int pid);

struct atTheBarbers {
    struct queue waiting_room;     // clients waiting in the queue

    int if_sleep;
    int wake_barber;        // flag if a client should wake up the barber
    int invitedPID;
    int shavedPID;

};

void init_barbershop(struct atTheBarbers* barbershop);
int increase_sem(int semID);
int decrease_sem(int senID);


#endif // GENERAL_H_INCLUDED
