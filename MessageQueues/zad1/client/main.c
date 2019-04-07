#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "systemV.h"

#define MAX_LINE_LEN 100
#define true 1
#define false 0

key_t client_key = -1;
int client_queueID = -1;

key_t server_key = -1;
int server_queueID = -1;

int PID = -1;
my_msg message;

int receive_STOP();


void handleSIGINT(int sig){

	struct msqid_ds buf;
 
	message.mtype = STOP;
    sprintf(message.mtext, "%6d", PID);

    msgsnd(server_queueID, &message, MAX_MSG_LEN, 0);  // send STOP message to server to delete client from array
	msgctl(client_queueID, IPC_RMID, &buf);
	exit(-1);     
}


int send_START(){

    sprintf(message.mtext,"%6d %d",PID, client_key);
    message.mtype = START;

    if (msgsnd(server_queueID, &message, MAX_MSG_LEN, 0) == -1) {
        return -1;
    }
    return 0;

}

int send_MIRROR (char* text){

    message.mtype = MIRROR;
    sprintf(message.mtext, "%6d %s", PID, text);
	

    if (msgsnd(server_queueID, &message, MAX_MSG_LEN, 0) == -1) {    // send MIRROR
        return -1;
    }

    if (msgrcv(client_queueID, &message, MAX_MSG_LEN, 0, 0) < 0) {
        return -1;
    }
    printf("%s\n",message.mtext);

    return 0;
}

int send_CALC (char* text) {
	
    message.mtype = CALC;
	
    sprintf(message.mtext, "%6d %s", PID, text);
	
    if (msgsnd(server_queueID, &message, MAX_MSG_LEN, 0) == -1) {
        return -1;
    }


    if (msgrcv(client_queueID, &message, MAX_MSG_LEN, 0, 0) < 0) {
        return -1;
    }
    printf("%s\n", message.mtext);

    return 0;
}


int send_TIME() {
    message.mtype = TIME;
    sprintf(message.mtext, "%6d", PID);

    if (msgsnd(server_queueID, &message, MAX_MSG_LEN, 0) == -1) {
        return -1;
    }


    if (msgrcv(client_queueID, &message, MAX_MSG_LEN, 0, 0) < 0) {
        return -1;
    }
    printf("%s\n",message.mtext);

    return 0;

}


int send_END() {

    struct msqid_ds buf;
    message.mtype = END;
    if (msgsnd(server_queueID, &message, MAX_MSG_LEN, 0) == -1){
        return -1;
    }
    msgctl(client_queueID, IPC_RMID, &buf);
    return 0;
}

int send_STOP() {

    struct msqid_ds buf;
    sprintf(message.mtext,"%6d", PID);
    message.mtype = STOP;
    message.mtype = STOP;
    if (msgsnd(server_queueID, &message, MAX_MSG_LEN, 0) == -1){
        return -1;
    }
    msgctl(client_queueID, IPC_RMID, &buf);
    return 0;
}


int return_key(){

    const char* homedir = getenv("HOME");

    key_t server_key = ftok(homedir, PROJECT);

    if (server_key == -1){
        printf("Error occured while creating KEY!\n");
        return -1;
    }
    return server_key;

}



int parse_messages(){

    int line_len;
    char line[MAX_LINE_LEN];
    int tmp;

    while(1){
		
		my_msg msg;
		if (msgrcv(client_queueID, &msg, MAX_MSG_LEN, 6, IPC_NOWAIT) > 0) receive_STOP();
		
        fgets(line, MAX_LINE_LEN, stdin);
        line_len = 0;
        char message[MAX_LINE_LEN] = "";

        int msg_end = 0;
        for(int i=0; i<MAX_LINE_LEN-1; i++) {
            if(!msg_end) message[i] = line[i];

            if(line[i] == ' ') {
				message[i] = '\0';
				msg_end =1;
            }

            if (line[i] == '\n') {
                line[i] = '\0';
                break;
            }
            line_len++;
        }
		
		#ifdef DEBUG
		printf("Line: %s\n", line);
        printf("Message: %s\n", message);
		#endif

        if (strncmp("MIRROR",message, 6) == 0) {
			
            tmp = send_MIRROR(&line[6]);
            if(tmp < 0) return tmp;
            continue;
        }

		if (strncmp("CALC",message, 4) == 0) {

            int i = 4;
            for( ; i<line_len; i++){

                if(line[i]!=' ') break;
            }

            if(i==4 || i == line_len) {
                printf("Parse error\n");
                continue;
            }

            tmp = send_CALC(&line[i]);
            if (tmp<0) return tmp;
            continue;
        }


        if( strncmp("TIME", message, 4) == 0)
        {

            tmp = send_TIME();
            if (tmp<0) return tmp;
            continue;
        } 
		

        if(strncmp("END",message, 3) == 0)
        {
            tmp = send_END();
			return tmp;
        }
		
		if(strncmp("STOP", message, 4) == 0){
			
			tmp = send_STOP();
            if(tmp<0) return tmp;
            break;
		}

    }//end while

    return tmp;

}


int live_client() {

	server_key = return_key();
    server_queueID = msgget(server_key, 0);

    if (server_queueID < 0) {
        printf("Cannot open server's queue!\n");
        return -1;
    }
    else {
        printf("Server queue opened!\n");
    }

	PID = getpid();

    
    client_key = server_key + PID;

    client_queueID = msgget(server_key + PID, IPC_EXCL | IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO );    // create client's queue

    if(client_queueID < 0) {

        printf("Error occured while creating client's queue!\n");
        return -1;
    }
    printf("Client's queue created with key: %d\n", client_key);

    send_START();

    int tmp = parse_messages();
    return tmp;
	
}


int receive_STOP() {

    struct msqid_ds buf;
    printf("\nServer conncection error. Receive Ctrl+C\n Exit");
    msgctl(client_queueID, IPC_RMID, &buf);

    exit(0);

}


int main()
{

    struct sigaction act;
    act.sa_handler = handleSIGINT;
    act.sa_flags = 0;
    sigset_t set;
    sigemptyset(&set);
    act.sa_mask = set;
    sigaction(SIGINT, &act, NULL);

    int tmp = live_client();
    if (tmp != 0) {
        printf("Error ocurred %d", errno);
	    struct msqid_ds buf;
        msgctl(client_queueID, IPC_RMID, &buf);
    }

    return 0;
}
