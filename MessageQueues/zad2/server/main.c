#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <pwd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "systemV.h"
#include <mqueue.h>
#include <string.h>


#define MAX_CLIENTS 20
#define DEBUG

mqd_t server_queue_dsc = -1;

Client clients_array[MAX_CLIENTS];
int clients_count = 0;
char server_que_name[50];
my_msg message;


int die() {

	message.mtype = STOP;
	for (int i=0; i<clients_count; i++){
		mq_send(clients_array[i].q_desc, (char*)&message, MAX_SEND_LEN, 2);
		mq_close(clients_array[i].q_desc);                 // only closed, not deleted yet
	}

	mq_close(server_queue_dsc);
	mq_unlink(server_que_name);
    return 0;

}

void handleSIGINT() {

    printf("\nSIGINT received\n");
	die();
	exit(-1);
}


int handle_START(){

	#ifdef DEBUG
    printf("START message received\n");
    #endif // DEBUG

    char client_q_name[40];
	message.mtext[6] = '\0';
	clients_array[clients_count].PID = atoi(message.mtext);
    sprintf(client_q_name, "/QueTest%6d", clients_array[clients_count].PID);
    int i=0;
    for(;i<strlen(client_q_name);i++)
        if(client_q_name[i]==' ')
            client_q_name[i]='_';
    int tmp = mq_open(client_q_name, O_WRONLY);
    if (tmp<0) {
        #ifdef DEBUG
        printf("failed to open with name '%s'",client_q_name);
        #endif // DEBUG
        return -1;
    }
    clients_array[clients_count].q_desc = tmp;
	clients_count++;
	return 0;

}

int handle_STOP() {
	#ifdef DEBUG
    printf("STOP message received\n");
    #endif // DEBUG

	message.mtext[6] = '\0';
	int clientPID = atoi(message.mtext);

	int i=0;
	for( ;i<clients_count;i++) {                      // find client
		if(clients_array[i].PID == clientPID)
        break;
	}

	mq_close(clients_array[i].q_desc);

	for( ; i <clients_count; i++) {
		clients_array[i].q_desc = clients_array[i+1].q_desc;
        clients_array[i].PID = clients_array[i+1].PID;
    }
	return 0;
}

int handle_MIRROR() {

	#ifdef DEBUG
    printf("MIRROR message received");
    #endif // DEBUG

	message.mtext[6]='\0';
    int clientPID = atoi(message.mtext);

	int i=0;
    for(i=0; i<clients_count; i++)                          // find client
		if(clients_array[i].PID == clientPID) break;

	int k;
    for(k=0; k<MAX_MSG_LEN-8;k++) {
		if(message.mtext[k+7] == '\0') break;                 // find text to reverse
    }

	k--;       // k+7 points to the last non \0 char

	my_msg stOut;
    stOut.mtype = MIRROR;

	int l;
    for(l=0;l<=k;l++) {
		stOut.mtext[l] = message.mtext[k+7-l];
    }

    stOut.mtext[k+1]='\0';
    int tmp = mq_send(clients_array[i].q_desc, (char*)(&stOut), MAX_SEND_LEN, 2);

	if(tmp<0) {
        die();
        //return -1;
    }

	return 0;
}


int handle_TIME(){
	#ifdef DEBUG
    printf("TIME message received");
    #endif // DEBUG

    message.mtext[6] = '\0';
    int clientPID = atoi(message.mtext);

	int i;
	for(i=0; i<clients_count; i++) {
		if(clients_array[i].PID == clientPID) break;
    }


    my_msg stOut;
    stOut.mtype = TIME;

	time_t cur_time;
	time(&cur_time);
    struct tm* tm_time;
    tm_time = localtime(&cur_time);
    strftime(stOut.mtext, MAX_MSG_LEN-1, "%Y.%m.%d %H:%M:%S", tm_time);

	int tmp = mq_send(clients_array[i].q_desc, (char*)(&stOut), MAX_SEND_LEN, 2);

	if(tmp<0) {
        die();
        //return -1;
    }


	return 0;
}


int handle_CALC() {

	#ifdef DEBUG
    printf("\nCALC message received\n");
    #endif // DEBUG

	message.mtext[6]='\0';
    int clientPID = atoi(message.mtext);

    int i, k;
    for(i=0; i<clients_count; i++) {
		if(clients_array[i].PID == clientPID) break;
    }

    for(k=7; k<MAX_MSG_LEN; k++) {
         if(message.mtext[k]=='+' || message.mtext[k]=='-' || message.mtext[k]=='*' || message.mtext[k] == '/' )  // find operator
         	break;
	}

	if(k == MAX_MSG_LEN) {
		 printf("Parse error, message without operator from client PID: %d\n",clients_array[i].PID);
         return 0;
	}

	char operator = message.mtext[k];
    message.mtext[k]='\0';

	int x = atoi(&message.mtext[7]);
    int y = atoi(&message.mtext[k+1]);
	int result;

	if (operator == '+') result = x+y;
    if (operator == '-') result = x-y;
    if (operator == '*') result = x*y;
    if (operator == '/') {
		if (y == 0) { printf ("Cannot devide by 0\n "); return 0; }
		result = x/y;
	}

	my_msg out;
	out.mtype = CALC;
    sprintf(out.mtext, "%d" ,result);

	int tmp = mq_send(clients_array[i].q_desc, (char*)(&out), MAX_SEND_LEN, 2);

	if(tmp<0) {
        die();
    }

	return 0;
}



int live_server() {

    for(int i=0;i<MAX_CLIENTS;i++)  {
        clients_array[i].PID = -1;
        clients_array[i].q_desc = -1;
    }


	sprintf(server_que_name, MAIN_QUE_NAME);
	printf("%s\n", server_que_name);
	struct mq_attr attr;
	attr.mq_flags = 0;
    attr.mq_msgsize = sizeof(my_msg)-1;
    attr.mq_maxmsg = MAX_MSG_COUNT;
    server_queue_dsc = mq_open(server_que_name, O_RDWR | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU, &attr);    // create server queue

	if (server_queue_dsc <0) {
        printf("Error while creating queue, errno: %d", errno);
        return -1;
    }
    printf("Server queue created and open");

    int if_end = 0;
    int if_empty = 0;

    while(!(if_end && if_empty)){

        ///------------------------------------------ tmp ----------------------------------

		int ret=mq_receive(server_queue_dsc, (char*)(&message), sizeof(my_msg), NULL);
        if(ret<0 && if_end)
            if_empty=1;
        else if(ret<0)
        {
            printf("if_end=%d,if_empty=%d\n",if_end,if_empty);
            perror("Error on read from main>");
        }



		#ifdef DEBUG
        printf("\nReceived message:  %s\n", message.mtext);
        #endif // DEBUG


        if (message.mtype == START) {
        	if (handle_START() != 0) return -1;
        }

        if (message.mtype==STOP) {
            if (handle_STOP() != 0) return -1;;
        }


        if (message.mtype == MIRROR) {
			if (handle_MIRROR() != 0) return -1;
        }


        if (message.mtype == TIME) {
            if (handle_TIME() != 0) return -1;
        }

        if (message.mtype == CALC) {
			if (handle_CALC() != 0) return -1;
		}


        if(message.mtype == END) {
            if_end=1;
            if(attr.mq_flags!=O_NONBLOCK)
            {
                if(mq_getattr(server_queue_dsc,&attr)<0)
                {
                    perror("Error on getting control structure>");
                    return -1;
                }
                attr.mq_flags=O_NONBLOCK;
                if(mq_setattr(server_queue_dsc,&attr,NULL)<0)
                {
                    perror("Error on setting control structure>");
                    return -1;
                }
            }
            #ifdef DEBUG
            printf("END message received");
            #endif // DEBUG
            if_end = 1;
        }
    }
    die();
	return 0;
}


int main() {

    struct sigaction act;
    act.sa_handler = handleSIGINT;
    act.sa_flags = 0;
    sigset_t set;
    sigemptyset(&set);
    act.sa_mask = set;
    sigaction(SIGINT, &act, NULL);

    int tmp = live_server();
    if (tmp != 0) {
        printf("Error ocurred    errno: %d", errno);
    }

    return 0;
}
