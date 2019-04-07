#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <semaphore.h>

int P = 0;    // producers number
int K = 0;    // consumers number
int N = -1;   // size of array

char input_name[50];
FILE* file_ptr = NULL;
int L =-1;              // length of line to compare
int search_mode;        // -1 search shorter, 0 equal, 1 longer
int write_mode;         // 0 default, 1 information about all consumers
int ns;                 // 0 read till EOF, >0 read during ns seconds

struct global_buf {

    char** array;           // array of pointers to lines
    int next_to_write;      // index of element to be written next by a producer
    int next_to_read;       // index of element to be read next by a costumer
} buf;

pthread_t* producers;
pthread_t* consumers;
int eof = -1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void SIGINT_handler(){
    exit(0);
}


int unlock_mutex(){                      // increase
    int tmp = pthread_mutex_unlock(&mutex);
    if (tmp != 0){
            perror("Error on unlocking mutex\n");
            exit(-1);
    }
    return 0;

}

int lock_mutex(){                      // decrease

    int tmp = pthread_mutex_lock(&mutex);
    if (tmp != 0){
            perror("Error while locking mutex\n");
            exit(-1);
    }
    return 0;
}


int compare_line(char* line){
    int length = strlen(line) -1;
    if (length > L) return 1;
    if (length == L) return 0;
    else return -1;
}

void* consumer(void* arg){
     // set pthread mask
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set,SIGTERM);
    if(pthread_sigmask(SIG_SETMASK, &set, NULL) == -1) {
        perror("Cannot set pthread mask.");
        exit(-1);
    }


    while (1){
        if (ns ==0 && eof == 1) break;

        if (write_mode == 1)
            printf("Consumer %lu: trying to lock mutex\n ",pthread_self());

        lock_mutex();

        if (write_mode == 1)
            printf("Consumer %lu: Trying to get a line \n ", pthread_self());

        if (buf.array[buf.next_to_read] == NULL){
            if(write_mode == 1)
                printf("Consumer %lu: nothing to get\n ", pthread_self());

            unlock_mutex();
            continue;
        }

        if(write_mode == 1)
            printf("Consumer %lu: read the line\n ", pthread_self());


        if(compare_line(buf.array[buf.next_to_read]) == search_mode){
         //   printf("LINE: %s",  buf.array[buf.next_to_read]);
            printf("Consumer %lu: found the correct line   index: %d line: %s\n", pthread_self(), buf.next_to_read, buf.array[buf.next_to_read]);
        }

        buf.array[buf.next_to_read] = NULL;
        buf.next_to_read++;

        if (buf.next_to_read == N) buf.next_to_read = 0;

        if (write_mode == 1){
            printf("Consumer: %lu unlocking mutex\n", pthread_self() );
        }
        unlock_mutex();
    }
    exit(0);

}



void* producer(void* arg){

    // set pthread mask
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set,SIGTERM);
    if(pthread_sigmask(SIG_SETMASK, &set, NULL) == -1) {
        perror("Cannot set pthread mask.");
        exit(-1);
    }


    while(!feof(file_ptr)){

        if (write_mode == 1)
            printf("Producer %lu: trying to lock mutex\n ",pthread_self());

        lock_mutex();

        if (write_mode == 1)
            printf("Producer %lu: got mutex\n", pthread_self());


        if (buf.array[buf.next_to_write] != NULL) {    // no empty space to write

            if(write_mode == 1)
                printf("Producer %lu: no empty space to write \nUnlock the mutex.\n", pthread_self());

            unlock_mutex();
            continue;
        }

        if (write_mode == 1)
            printf("Producer %lu: write line to array\n", pthread_self());

        char* line = calloc(120, sizeof(char));

        if (fgets(line, 120, file_ptr) == NULL){
            if (feof(file_ptr)) break;
            perror("Cannot read line from file");
            unlock_mutex();
            exit(-1);
        }


        buf.array[buf.next_to_write] = line;
        buf.next_to_write++;

        if (buf.next_to_write == N) buf.next_to_write = 0;


        if (write_mode == 1){
            printf("Producer: %lu unlocking mutex\n", pthread_self() );
        }
        unlock_mutex();
    }

    exit(0);

}


int main(int argc, char **argv){

    if (argc != 2){
        printf("Give an input file name.");
        exit(0);
    }
    else {
        
        file_ptr = fopen(argv[1], "r");
        if (file_ptr == NULL){
            perror("Cannot open args file");
            exit(-1);
        }
            
        fscanf(file_ptr, "PRODUCERS %d", &P);
        fscanf(file_ptr, "\nCONSUMERS %d", &K);  
        fscanf(file_ptr, "\nN %d", &N);
        fscanf(file_ptr, "\nFILE_NAME %s", input_name);
        fscanf(file_ptr, "\nL %d", &L);
        fscanf(file_ptr, "\nSEARCH_MODE %d", &search_mode);
        fscanf(file_ptr, "\nWRITE_MODE %d", &write_mode);
        fscanf(file_ptr, "\nNS %d", &ns);     
        

        // alloc memory
        producers = malloc(P * sizeof(pthread_t));
        for (int i=0; i<P; i++) producers[i]= -1;

        consumers = malloc(K * sizeof(pthread_t));
        for (int i=0; i<K; i++) consumers[i]= -1;

        buf.array = malloc(N * sizeof(char*));
        buf.next_to_read = 0;
        buf.next_to_write =0;



        for (int i=0; i<P; i++){
            pthread_create(producers+i, NULL, producer, NULL);
        }

        for (int i=0; i<K; i++){
            pthread_create(consumers+i, NULL, consumer, NULL);
        }

        if (ns >0) {
            sleep(ns);
            fclose(file_ptr);
            exit(0);
         }

       for (int i=0; i<P; i++){
            pthread_join(producers[i], NULL);
        }

        for (int i=0; i<K; i++){
            pthread_join(consumers[i], NULL);
        }

        fclose(file_ptr);
        exit(0);
     
    }

}
