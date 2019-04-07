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
#include "systemV.h"

#define MAX_CLIENTS 20
#define DEBUG

int server_queueID = -1;
Client clients_array[MAX_CLIENTS];
int clients_count = 0;
my_msg message;
struct msqid_ds buf;


void handleSIGINT()
{
    
    printf("\nSIGINT received\n");
	my_msg msg;
	msg.mtype = STOP;
	for (int i=0; i<clients_count; i++){    
		msgsnd(clients_array[i].queueID, &msg, MAX_MSG_LEN, 0);		
	} 
	
	struct msqid_ds buf;
	msgctl(server_queueID, IPC_RMID, &buf);
    exit(-1);
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


int handle_START(){
	#ifdef DEBUG
    printf("START message received\n");
    #endif // DEBUG

    if(clients_count == MAX_CLIENTS) {
		printf("Exceeded max client number\n");
        msgctl(server_queueID, IPC_RMID, &buf);
        return -1;
    }

    message.mtext[6]='\0';
    clients_array[clients_count].PID = atoi(message.mtext);
    clients_array[clients_count].queueID = atoi(message.mtext + 7);

    int tmp = msgget(clients_array[clients_count].queueID, 0);         
    if(tmp < 0) {
		printf("Cannot open client queue. Client PID: %d", clients_array[clients_count].PID);
        msgctl(server_queueID, IPC_RMID, &buf);
        return -1;
    }
			
    clients_array[clients_count].queueID = tmp;
	clients_count++;
	return 0;
}
	
int handle_STOP() {
	#ifdef DEBUG
    printf("STOP message received\n");
    #endif // DEBUG
        
	int clientPID = atoi(message.mtext);
          
	int i=0;
	for( ;i<clients_count;i++) {                      // find client
		if(clients_array[i].PID == clientPID)
        break;
	}
		
           
	for( ; i <clients_count; i++) {
		clients_array[i].queueID = clients_array[i+1].queueID;
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
    int tmp = msgsnd(clients_array[i].queueID, &stOut, MAX_MSG_LEN, 0);
            
	if(tmp<0) {
        msgctl(server_queueID, IPC_RMID, &buf);
        return -1;
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
            
	int tmp = msgsnd(clients_array[i].queueID, &stOut, MAX_MSG_LEN, 0);
    if(tmp <0) {
		msgctl(server_queueID, IPC_RMID, &buf);
        return -1;
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
			
    if (msgsnd(clients_array[i].queueID, &out, MAX_MSG_LEN, 0) ) {
		msgctl(server_queueID, IPC_RMID, &buf);
        return -1;
	}	
	return 0;		
}	



int live_server() {

    for(int i=0;i<MAX_CLIENTS;i++)  {
        clients_array[i].PID=-1;
        clients_array[i].queueID=-1;
    }

    key_t server_key = return_key();
    server_queueID = msgget(server_key, IPC_EXCL | IPC_CREAT | S_IRWXU | S_IRWXG |S_IRWXO);    // create queue
    if (server_queueID <0) {
        printf("Error while creating queue, errno: %d", errno);
        return -1;
    }
    printf("Server queue created and open, key: %d\n", server_key);

    int if_end = 0;
    int if_empty = 0;
   
    while(!(if_end && if_empty)){
      
		if(if_end) {
            msgrcv(server_queueID, &message, MAX_MSG_LEN, 0, IPC_NOWAIT);
			if (errno == ENOMSG) break;
        }

        else {
            if (msgrcv(server_queueID, &message, MAX_MSG_LEN, 0, MSG_NOERROR) == -1){

            if (errno != ENOMSG) {
				printf("Queue not empty, error occured. Errno: %d\n",errno);
                msgctl(server_queueID, IPC_RMID, &buf);
                return -1;
            }
				
            else {
				if_empty = 1;                        // queue is empty
                continue; }
        	}
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
            #ifdef DEBUG
            printf("END message received");
            #endif // DEBUG
            if_end = 1;
        }
    }

    msgctl(server_queueID, IPC_RMID, &buf);
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
