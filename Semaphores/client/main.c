#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include "general.h"
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

int clients_num = 3;
int shavings_num = 2;

struct atTheBarbers* sh_barbershop ;
int semID;
int sh_barbID;       // share memory ID

//#define DEBUG

void shared_mem_get(){

    const char* home_env = getenv("HOME");
    key_t shm_key = ftok(home_env, BARBER_ID);

    sh_barbID = shmget(shm_key, 0, 0);
    sh_barbershop = (struct atTheBarbers*) shmat(sh_barbID, NULL, 0);
}


void sem_get(){

    key_t key = ftok("/tmp",'s');
    semID = semget(key, 1, 0);

    if (semID == -1) {
        perror("Failure getting semaphore ID.");
        exit(-1);
    }
}


void handleSIGINT(int sig) {
    exit(0);
}

void end(){

}


int goToBarber(){

    int while_shaving = 0;
    int in_queue = 0;
    int is_invited = 0;

    for(int i=0; i<shavings_num; i++){

        while(1) {

            #ifdef DEBUG
            printf("SEM VAL1  PID: %d\t  %d\n", getpid(), semctl(semID, 0, GETVAL));
            #endif // DEBUG

            decrease_sem(semID);

            #ifdef DEBUG
            printf("SEM VAL2  PID: %d\t  %d\n", getpid(), semctl(semID, 0, GETVAL));
            #endif // DEBUG

            if(while_shaving == 1 && sh_barbershop->shavedPID == getpid()){    // barber finished shaving

                while_shaving = 0;
                sh_barbershop->invitedPID = -1;
                in_queue = 0;
                is_invited = 0;
                #ifdef DEBUG
                printf("SEM VAL3  PID: %d\t  %d barber finished shaving\n", getpid(), semctl(semID, 0, GETVAL));
                #endif
                increase_sem(semID);

                char out[100];
                sprintf(out, "Barber finished %d shaving ", i+1);
                print_time1(out, getpid());
                break;     // end shaving
            }

            if (sh_barbershop->if_sleep == 1 && sh_barbershop->wake_barber == 0 && in_queue == 0 && is_invited == 0){
                                                                // barber is sleeping, nobody wakes him up and client've just visited barbershop
                while_shaving = 1;
                sh_barbershop->wake_barber = 1;                         // start shaving
                sh_barbershop->invitedPID = getpid();
                sh_barbershop->shavedPID = getpid();

                print_time1("Client wakes up the barber\n", getpid());
                print_time1("Client is taking the seat\n", getpid());
                increase_sem(semID);

                continue;
            }

            if (in_queue ==  0) {     // take a seat in the queue

                if (enqueue(&(sh_barbershop->waiting_room), getpid()) == 1) {    // check if queue is full

                    in_queue = 1;
                    #ifdef DEBUG
                    printf("SEM VAL3  PID: %d\t  %d  take a seat in the waiting room\n", getpid(), semctl(semID, 0, GETVAL));
                    #endif

                    print_time1("Client is taking a seat in the waiting room ", getpid());
                    increase_sem(semID);
                    continue;
                }

                else {      // queue is full
                    #ifdef DEBUG
                    printf("SEM VAL3  PID: %d\t  %d  come back later\n", getpid(), semctl(semID, 0, GETVAL));
                    #endif
                    increase_sem(semID);

                    while_shaving = 0;
                    in_queue = 0;
                    is_invited = 0;

                    print_time1("Queue is full, come back later", getpid());

                    continue;
                }
            }

            if (in_queue == 1 && is_invited == 0) {

                #ifdef DEBUG
                printf("Checking if the barber invited the client %d\n", getpid());
                #endif // DEBUG

                if (sh_barbershop->invitedPID == getpid()){        // inivited
                    print_time1("Client was invited ", getpid());
                    is_invited = 1;
                    sh_barbershop->invitedPID = getpid();
                    increase_sem(semID);
                    continue;

                }

                else {                             // not invited
                    #ifdef DEBUG
                    printf("SEM VAL3  PID: %d\t  %d  still wait in the queue\n", getpid(), semctl(semID, 0, GETVAL));
                    #endif
                    increase_sem(semID);

                    continue;
                }
            }

            if (in_queue == 1 && is_invited == 1 && sh_barbershop->invitedPID == getpid()) {


                sh_barbershop->shavedPID = getpid();            // respond to barber
                while_shaving = 1;
                print_time1("Client is taking a seat ", getpid());

                #ifdef DEBUG
                printf("SEM VAL3  PID: %d\t  %d   start getting cut\n", getpid(), semctl(semID, 0, GETVAL));
                #endif

                increase_sem(semID);
                continue;
            }

			exit(-1);
        }
    }
    exit(0);
}



int create_clients(){

    pid_t PID;
    int clientsPID[clients_num];

    for (int i = 0; i<clients_num; i++){

        PID = fork();
        if (PID == 0) {
            printf("New client %d\n", getpid());
            goToBarber();
        }
        else {
            clientsPID[i]=  PID;
        }
    }

    for (int i=0; i<clients_num; i++){
        wait(NULL);
    }
    return 0;
}


int main(int argc, char **argv) {

    if (argc != 3){
       // printf("Not enough argumets! Using default ones.\n");
    }
    else {
        clients_num = atoi(argv[1]);
        shavings_num = atoi(argv[2]);
        if (clients_num <= 0 || shavings_num <= 0) {
            printf("Incorrect arguments.\n");
            exit(-1);
        }
    }
    signal(SIGINT, handleSIGINT);
    shared_mem_get();
    sem_get();

    char next_clients[100];
    next_clients[0]='y';

    while (next_clients[0] == 'y'){
        create_clients();
        printf("next clients? y/n");
        fgets(next_clients, 100 ,stdin);
        printf("\n");
    }

    return 0;
}
