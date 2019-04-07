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
#include <signal.h>
#include <time.h>

//#define DEBUG


int sits_num = 10;
int semID;
int sh_barbID;
struct atTheBarbers* sh_barbershop;


void shared_mem_init(){

    const char* home_env = getenv("HOME");
    key_t shm_key = ftok(home_env, BARBER_ID);

    sh_barbID = shmget(shm_key, sizeof(struct atTheBarbers), IPC_CREAT | 0666);        // add flag?
    sh_barbershop = (struct atTheBarbers*) shmat(sh_barbID, NULL, 0);

    init_barbershop(sh_barbershop);
}

void handleSIGINT(int sig)
{
    printf("Received signal");
    exit(0);
}

void end(){          // free resources

    if(shmctl(sh_barbID, IPC_RMID, NULL)<0){
        perror("error occured while deleting shared memory segment");
    }

    if(semctl(semID,0,IPC_RMID) <0){
        perror("error occred while deleting semaphore");
    }
    exit(0);
}


void barber_life(){

    int whileShaving = 0;      // flag if a client is being shaved

    while (1) {

        #ifdef DEBUG
        printf("SEM VAL1:  %d\n   %d", semctl(semID, 0, GETVAL, getpid()));
        #endif
        decrease_sem(semID);
        #ifdef  DEBUG
        printf("SEM VAL2:  %d\n   %d", semctl(semID, 0, GETVAL, getpid()));
        #endif // DEBUG


        if(whileShaving == 1)
        {
                whileShaving = 0;       // end shaving


                print_time1("Just've finished shaving client", sh_barbershop->shavedPID);

                sh_barbershop->shavedPID = -1;
			    sh_barbershop->invitedPID = -1;
                increase_sem(semID);
                continue;
        }

        if(sh_barbershop->if_sleep == 1){

            if(sh_barbershop->wake_barber == 1){     // barber is sleeping and received signal to wake up

                sh_barbershop->if_sleep = 0;
                sh_barbershop->wake_barber = 0;

                print_time("Barber just woke up");


                char out[100];
                whileShaving = 1;
                sprintf(out,"Start shaving client: %d", sh_barbershop->shavedPID);

                increase_sem(semID);
                continue;
            }
            else {            // sleep more
                increase_sem(semID);
                continue;
            }
        }


        if (sh_barbershop->shavedPID == -1) {   // barber isn't sleeping and shaving a client

            if(get_que_size(&(sh_barbershop->waiting_room)) <=0 && sh_barbershop->invitedPID == -1){    // fall asleep

                print_time("Barber is falling asleep");
                sh_barbershop->if_sleep = 1;

                increase_sem(semID);
                continue;
            }

            if (sh_barbershop->invitedPID == -1){    // waiting room is not empty so invite a client

                int clientPID = dequeue(&(sh_barbershop->waiting_room));
                if (clientPID > 0) {
                    sh_barbershop->invitedPID = clientPID;

                    char out[200];
                    sprintf(out, "New client invited: %d", clientPID);
                    print_time(out);
                }

                increase_sem(semID);
                continue;
            }
            increase_sem(semID);
            continue;

        }

        if(sh_barbershop->invitedPID >0 && sh_barbershop->shavedPID >0 )   // invited and responded

                whileShaving = 1;

                char out[100];
                sprintf(out,"Start shaving client");
                print_time1(out, sh_barbershop->shavedPID);

                increase_sem(semID);
                continue;
        }

}


void sem_init(){

    union semun {
        int val;
        struct semid_ds *buf;
        ushort array [1];
    } sem_attr;

    sem_attr.val = 1;
    key_t key = ftok("/tmp",'s');
    semID = semget(key, 1, IPC_CREAT  | 0666);

    if (semID == -1) {
        perror("Failure creating semaphore.");
        exit(0);
    }

    if (semctl(semID, 0, SETVAL, sem_attr) == -1){
        perror("Error while setting semaphore value.");
        exit(0);
    }
}


int main(int argc, char **argv) {

    if (argc != 2){
     //   printf("Not enough argumets! Using default ones.\n");
    }
    else {
        sits_num = atoi(argv[1]);
    }

    signal(SIGINT, handleSIGINT);
    atexit(end);

    shared_mem_init();
    sem_init();
    barber_life();

    return 0;
}
