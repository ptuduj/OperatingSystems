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
#define _GNU_SOURCE
#include "general.h"

int sock_fd;
char name[40];                 // socket name
char type;                     // socket type
char* socket_path;             // local socket path

int port_num;
char* ipv4_addr;
struct sockaddr* addr;


void clean(){

    free(addr);
    shutdown(sock_fd, SHUT_RDWR);
    close(sock_fd);
}


void handleSIGINT() {
    exit(0);
}




int init_local_socket(void){

	sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("Creating socket for local communication failed.\n");
        exit(-1);
    }

    struct sockaddr_un* local_addr = malloc(sizeof(struct sockaddr_un));
    local_addr->sun_family = AF_UNIX;
    strcpy(local_addr->sun_path, socket_path);             // path to UNIX socket

    addr = (struct sockaddr *) local_addr;
    return 0;
 }

int init_net_socket(void){

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("Creating socket for network communication failed.");
        exit(-1);
    }
    else printf("Socked created.");

    struct sockaddr_in* sockaddr_net= malloc(sizeof(struct sockaddr_in)) ;
    sockaddr_net->sin_family = AF_INET;
    sockaddr_net->sin_port = htons(port_num);               // to network byte order
    if (inet_aton(ipv4_addr, &sockaddr_net->sin_addr) == 0){
        perror("Invalid ipv4 address.\n");
        exit(-1);
    }

    addr = (struct sockaddr *) sockaddr_net;

    return 0;
 }


int main(int argc, char *argv[]) {

    if(argc < 4 || argc > 6) {
        printf("Usage for local socket:     <socket name> <l> <path>\n");
        printf("Usage for network socket:   <socket name> <n> <IPv4 address> <port number>\n");
        return(-1);
    }
    atexit(clean);


    if (argc == 4){                                         // lockal socket
        if (strcmp(argv[2], "l") != 0) {
            printf("Invalid arguments.\n");
            return -1;
        }

        type = 'l';
        strcpy(name, argv[1]);
        socket_path= argv[3];

        init_local_socket();
    }

    else {                                                    // network socket
        if (strcmp(argv[2], "n") != 0) {
            printf("Invalid arguments.\n");
            return -1;
        }

        type = 'n';
        strcpy(name, argv[1]);
        ipv4_addr = argv[3];
        port_num = atoi(argv[4]);
        init_net_socket();
    }


    struct sigaction act;                         // handle SIGINT
    act.sa_handler = handleSIGINT;
    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    act.sa_flags = 0;


    if (connect(sock_fd, addr, sizeof(* addr)) == -1) {
        perror("Connection error.\n");
        exit(-1);
    }


    printf("%s", name);
    if (send(sock_fd, name, sizeof(name), 0) == -1){         // send name to server
        perror("Cannot send socket name to server");
        exit(-1);
    }
    else printf("Name sent.\n");



    int regist_result;         // 0 registered      -1 not
    if(recv(sock_fd, &regist_result, sizeof(int), 0) == -1) {
        perror("RECV registration reply error.\n");
        exit(-1);
    }
    if (regist_result == -1){
        printf("Registration failed.\n");
        exit(-1);
    }
    else printf("Registration successful.\n");




    while(1)
    {
       // printf("WORKING\n");
        struct Message msg;
        if (recv(sock_fd, &msg, sizeof(msg), 0) == -1){
            perror("Error on recieving message from server.\n");
            exit(-1);
        }


        struct Reply reply;
        reply.number = msg.number;

        if (msg.operator == ADD){
            reply.result = msg.arg1 + msg.arg2;
            printf("Received message nr %d: ADD %d %d\n", msg.number, msg.arg1, msg.arg2);
        }

        else if (msg.operator == SUB) {
            reply.result = msg.arg1 - msg.arg2;
            printf("Received message nr %d: SUB %d %d\n", msg.number, msg.arg1, msg.arg2);
        }

        else if(msg.operator == MUL) {
            reply.result = msg.arg1 * msg.arg2;
            printf("Received message nr %d: MUL %d %d\n", msg.number, msg.arg1, msg.arg2);
        }

        else if(msg.operator == DIV) {
            reply.result = msg.arg1 / msg.arg2;
            printf("Received message nr %d: DIV %d %d\n", msg.number, msg.arg1, msg.arg2);
        }
        else if(msg.operator == PING){
		;
        }

        strcpy(reply.name, name);

        if(send(sock_fd, &reply, sizeof(reply), 0) == -1)
            perror("Error on sending reply message.\n");
    }

    return 0;
}
