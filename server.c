#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "general.h"

#define MAX_EVENTS 10

struct Client{
    char* name;
    int fd;                     // socket fd for client-server communication
    struct sockaddr addr;       // client socket address
    socklen_t addr_len;
};

struct Client* clients[MAX_CLIENTS];             // list of clients
int msg_count = 0;
int efd1;                                        // epoll file discriptor
int efd2;
struct Message msg;

int sockfd_net;
int sockfd_local;

pthread_t thread_input;
pthread_t thread_network;
pthread_t thread_ping;

int new_msg = 0;
sem_t ping_sem;


void clean(){

    shutdown(sockfd_net, SHUT_RDWR);
    close(sockfd_net);
    shutdown(sockfd_local, SHUT_RDWR);
    close(sockfd_local);
}


void handleSIGINT() {
    printf("handle SIGINT/n");
    exit(0);
}

void* task_input(void *args){

    while(1) {

        printf("Input thread working\n");
        int arg1, arg2;
        int operator;
        char buf[5];

        scanf("%s %d %d", buf, &arg1, &arg2);
 
        if (strcmp(buf, "add") == 0) msg.operator = ADD;
        else if (strcmp(buf, "sub") == 0) msg.operator = SUB;
        else if (strcmp(buf, "mul") == 0) msg.operator = MUL;
        else if (strcmp(buf, "div") == 0) msg.operator = DIV;

        else {
            printf("Invalid opeartor, use: add, sub, mul, div\n");
            continue;
        }

        new_msg = 1;
        msg.arg1 = arg1;
        msg.arg2 = arg2;
		
        msg.number = msg_count;
	msg_count++;
    }

}


int add_new_client(int sockfd, char* client_name, struct sockaddr addr) {

    int i;
    for (i=0; i<MAX_CLIENTS; i++){
        if (clients[i] == NULL) break;
    }
    if (i == MAX_CLIENTS){
        printf("MAXIMUM number of clients. Cannot add one more.\n");
        return -1;
    }
    clients[i] = malloc(sizeof(struct Client));
    clients[i]->name = client_name;
    clients[i]->fd = sockfd;
    clients[i]->addr = addr;
    return 0;

}


struct Client* find_client() {

    for (int i=0; i<MAX_CLIENTS; i++){
        if(clients[i] != NULL) return clients[i];
    }
    return NULL;
}


void* task_network (void *args) {


    while(1) {
      //  printf("Network thread working\n");
        struct epoll_event events[MAX_EVENTS];

        // check if a new client want to register, don't wait if no event occured
        int epoll_result = epoll_wait(efd1, events, MAX_EVENTS, 0);

        sem_wait(&ping_sem);
        if(epoll_result > 0) {

            int sockfd = events[0].data.fd;                      // local or network server socket

            struct sockaddr client_addr;
            socklen_t addr_len = sizeof(client_addr);

            int client_fd = accept(sockfd, &client_addr, &addr_len);       // new socket for client-server communication

            if (client_fd == -1){
                perror("Accepting connection with a new client failed.\n");
                continue;
            }

            // receive clients name
            char client_name[50];

            if (recv(client_fd, client_name, 50, 0) == -1) perror("Error while receiving client's name.");

            else {

                 int if_registered = add_new_client(client_fd, client_name, client_addr);
                 if (if_registered == 0) {
                    printf("New client registerd: %s  fd %d.\n", client_name, client_fd);
              
                if (send(client_fd, &if_registered, sizeof(int), 0) == -1)
                    perror("Problem with sending registration result.\n");
                }

            }
       }


        if (new_msg == 1){
            struct Client* client = find_client();

                                    // don't ping client concurrently
            if (client == NULL){
                sem_post(&ping_sem);
                continue;
            }
            struct Message* msg1 = &msg;
            
            if (send(client->fd, msg1, sizeof(msg), MSG_NOSIGNAL) == -1){
                perror("SEND error.\n");
                exit(-1);
            }
            else {
                printf("Sent message nr %d\n", msg1->number);

                struct Reply reply;
                reply.number = 0;
                reply.result = 0;
                if (recv(client->fd, &reply, sizeof(reply), 0) == -1){
                    perror("RECV reply error.\n");
                    exit(-1);
                }
                else
                    printf("Got reply nr %d from %s: %d\n", reply.number, reply.name, reply.result);
            }
            new_msg = 0;
        }
        sem_post(&ping_sem);
    }
    return NULL;

}

// thread checking if clients are still responding
void *task_ping(void *args)
{
    printf("Ping thread working\n");

    while(1){

       sem_wait(&ping_sem);
       struct Message ping_msg;
        ping_msg.operator = PING;
        struct Reply reply;

        for(int i = 0; i < MAX_CLIENTS; i++){

            int to_remove = 0;
            if(clients[i] != NULL) {
               if (send(clients[i]->fd, &ping_msg, sizeof(ping_msg), MSG_NOSIGNAL) == -1) to_remove = 1;
               if (recv(clients[i]->fd, &reply, sizeof(reply), 0) == -1) to_remove = 1;
                if(to_remove == 1) {
                    printf("Client: %s removed.\n", clients[i]->name);
           	        close(clients[i]->fd);
                        free(clients[i]);
                        clients[i] = NULL;

                }
            }
        } 
     sem_post(&ping_sem);
    }  
 

}


int init_local_socket(char* path){

    sockfd_local = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd_local == -1) {
        perror("Creating socket for local communication failed.\n");
         exit(-1);
    }

    struct sockaddr_un local_addr;
    local_addr.sun_family = AF_UNIX;
    strcpy(local_addr.sun_path, path);             // path to UNIX socket


    if (bind(sockfd_local, (struct sockaddr *) &local_addr, sizeof(local_addr)) == -1){
        perror("Binding socket for local communication failed.\n");
        exit(-1);
    }

    if (listen(sockfd_local, MAX_CLIENTS) == -1){
	perror("Listen failed.\n");
	exit(-1);
    }

	return 0;
 }

int init_net_socket(int port_num){

    sockfd_net = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_net == -1) {
        perror("Creating socket for network communication failed.");
         exit(-1);
    }


    // network socket address
    struct sockaddr_in sockaddr_net;
    sockaddr_net.sin_family = AF_INET;
    sockaddr_net.sin_port = htons(port_num);               // to network byte order
    sockaddr_net.sin_addr.s_addr = INADDR_ANY;


    if (bind(sockfd_net, (struct sockaddr *) &sockaddr_net, sizeof(sockaddr_net)) == -1) {
        perror("Binding socket for network communication failed.");
        exit(-1);
    }

    if (listen(sockfd_net, MAX_CLIENTS) == -1){
	perror("Listen failed\n");
	exit(-1);
    }

	 return 0;
 }


int init_epoll(){

	efd1 = epoll_create1(0);
	if (efd1 < 0){
		perror("Creating epoll instance failed.\n");
		exit(-1);
	}

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = sockfd_local;

    // watch socket for network and local connections
	if(epoll_ctl(efd1, EPOLL_CTL_ADD, sockfd_local, &event) != 0) {
        perror("EPOLL: Error on registering event.\n");
        exit(-1);
    }

    event.data.fd = sockfd_net;
    if(epoll_ctl(efd1, EPOLL_CTL_ADD, sockfd_net, &event) != 0) {
        perror("EPOLL: Error on registering event.\n");
        exit(-1);
    }

	return 0;
}


int main(int argc, char *argv[]) {

	if (argc < 3) {
        printf("Not enough arguments. Usage: <port number> <UNIX socket path>.\n");
        return -1;
    }


     struct sigaction act;                         // handle SIGINT
     act.sa_handler = handleSIGINT;
     sigset_t mask;
     sigfillset(&mask);
     sigdelset(&mask, SIGINT);
     act.sa_flags = 0;
     sigaction(SIGINT, &act, NULL);
     atexit(clean);

     init_local_socket(argv[2]);
     init_net_socket(atoi(argv[1]));

     init_epoll();
     sem_init(&ping_sem, 0, 1);


    if (pthread_create(&thread_input, NULL, task_input, NULL) != 0 || pthread_create(&thread_network, NULL, task_network, NULL) != 0 ||
        pthread_create(&thread_ping, NULL, task_ping, NULL) != 0) {
	    perror("Creating pthreads failed.\n");
            exit(-1);
    }

    while(1) {}

    return 0;
}
