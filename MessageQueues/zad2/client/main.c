#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include "systemV.h"
#include <string.h>

#define MAX_LINE_LEN 100

//#define DEBUG 1

char q_name[50];         // client queue name
mqd_t cl_queue_dsc;
mqd_t server_queue_dsc;
my_msg message;
int PID = -1;


int die(){
    #ifdef DEBUG
    printf("Dying");
    #endif // DEBUG
	mq_close(server_queue_dsc);
	mq_unlink(q_name);
    exit(0);
}

void handleSIGINT(int sig){

	message.mtype = STOP;
    sprintf(message.mtext, "%6d", PID);

    mq_send(server_queue_dsc, (char*)&message, MAX_SEND_LEN, 2);  // send STOP message to server to delete client from array
	die();
}


int send_START(){

    message.mtype = START;
    sprintf(message.mtext, "%6d", getpid());
    printf("My PID: %s\n", message.mtext);
    sleep(1);

	if (mq_send(server_queue_dsc, (char*)&message, MAX_SEND_LEN, 2) == -1) {
        return -1;
    }
	return 0;

}

int send_MIRROR (char* text){

	message.mtype = MIRROR;
	sprintf(message.mtext, "%6d %s", PID, text);

    if (mq_send(server_queue_dsc, (char*)&message, MAX_SEND_LEN, 2) == -1) {
        return -1;
    }

    if (mq_receive(cl_queue_dsc, (char*)(&message), MAX_MSG_LEN, NULL) == -1){
		return -1;
	}

    printf("%s\n",message.mtext);
    return 0;
}

int send_CALC (char* text) {

    message.mtype = CALC;
	sprintf(message.mtext, "%6d %s", PID, text);

    if (mq_send(server_queue_dsc, (char*)&message, MAX_SEND_LEN, 2) == -1) {
        return -1;
    }

    if (mq_receive(cl_queue_dsc, (char*)(&message), MAX_MSG_LEN, NULL) == -1){
		return -1;
	}

    printf("%s\n",message.mtext);
    return 0;
}


int send_TIME() {
    message.mtype = TIME;
    sprintf(message.mtext, "%6d", PID);

	if (mq_send(server_queue_dsc, (char*)&message, MAX_SEND_LEN, 2) == -1) {
        return -1;
    }

    if (mq_receive(cl_queue_dsc, (char*)(&message), MAX_MSG_LEN, NULL) == -1){
		return -1;
	}

    printf("%s\n",message.mtext);
    return 0;
}


int send_END() {

    message.mtype = END;

	if (mq_send(server_queue_dsc, (char*)&message, MAX_SEND_LEN, 2) == -1) {
        return -1;
    }

	// end client queue
    return 0;
}

int send_STOP() {

    sprintf(message.mtext,"%6d", PID);
    message.mtype = STOP;
    if (mq_send(server_queue_dsc, (char*)&message, MAX_SEND_LEN, 2) == -1) {
        return -1;
    }

    // end client queue
    return 0;
}



int parse_messages(){
    #ifdef DEBUG
    printf("in prase_messages\n");
    #endif // DEBUG

    int line_len;
    char line[MAX_LINE_LEN];
    int tmp;

    while(1){
        #ifdef DEBUG

        #endif // DEBUG
		
	char ptr[MAX_MSG_LEN];
	struct mq_attr attr;
        mq_getattr(cl_queue_dsc, &attr);                 // check if server ended and client has to die
        attr.mq_flags = O_NONBLOCK;
        mq_setattr(cl_queue_dsc, &attr, NULL);

        #ifdef DEBUG
        printf("checking if STOP form server\n");
        #endif // DEBUG
	if (mq_receive(cl_queue_dsc, ptr, MAX_MSG_LEN, NULL) >= 0) die();      // client received STOP from server

        mq_getattr(cl_queue_dsc, &attr);
        attr.mq_flags = 0;
        mq_setattr(cl_queue_dsc, &attr, NULL);
	
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

    sprintf(q_name,"/QueTest%6d",getpid());               // change queue name
    int i=0;
    for(;i<strlen(q_name);i++)
        if(q_name[i]==' ')
            q_name[i]='_';
	PID = getpid();

    struct mq_attr q_attr;
    q_attr.mq_flags = 0;
    q_attr.mq_msgsize = MAX_MSG_LEN-1;
    q_attr.mq_maxmsg = MAX_MSG_COUNT;
	cl_queue_dsc = mq_open(q_name, O_RDONLY | O_CREAT | O_EXCL, S_IRWXO | S_IRWXG | S_IRWXU, &q_attr);    //opening to read only

	#ifdef DEBUG
	printf("Opened Que with name:'%s'",q_name);
	#endif // DEBUG
	if (cl_queue_dsc == -1) {
        printf("Cannot create client queue!\n");
        perror("Error>");
		return -1;
    }

    char* s_name = MAIN_QUE_NAME;
	server_queue_dsc = mq_open(s_name, O_WRONLY);
    if(server_queue_dsc == -1) {
		printf("Cannot open server queue\n");
        perror("Error>");
        return -1;
    }

    send_START();

    int tmp = parse_messages();
    return tmp;

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
	    die();
    }

    return 0;
}
