#include "commonFunc.h"


SOCKET multicast_sock;
struct sockaddr_in multicast_addr;

SOCKET requets_listener;
int spec_node = FALSE;
int current_tokenId;

char* clientId;
int listenerPort;
char* neighIP;
int neighPort;
int startToken;
char* protocol;
int last_seen_msgID = 0;


char receiver[MAX_NAME_LEN];
char msg[MAX_MSG_LEN];
int is_msg_in_buf = FALSE;
int last_sender = FALSE;


void start_winsock(){   
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    int err;  

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {                              
        printf("WSAStartup failed with error: %d\n", err);
        exit(-1);
    } 
}

void check_winsock_last_error(){
    printf("%d",  WSAGetLastError());     
}

SOCKET init_socket_udp(){
  SOCKET sock =  socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == INVALID_SOCKET){
      printf("Error on create udp socket\n");
      check_winsock_last_error();
  }
  return sock;
}


void create_multicast_socket(){
    multicast_sock = init_socket_udp();
    if (multicast_sock == INVALID_SOCKET) exit(-1) ;
    
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr("224.1.1.2");
    multicast_addr.sin_port = htons(5007);
}


void _log(){
    char log[100];
    memset(log,0,100);
    char buff[20];
    time_t now = time(NULL);

    
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
    strcpy(log, buff);
    strcat(log, " ");
    strcat(log, clientId); 
   // sprintf(log,"%ld %s",now,clientId);
    
  //  printf("%s\n", log);    
    if (sendto(multicast_sock, log, sizeof(log), 0, (SOCKADDR*) &multicast_addr, sizeof(multicast_addr)) == SOCKET_ERROR){
        //printf("Error on sendto to log\n");
        //scheck_winsock_last_error();
    }       
}


void check_if_valid_tokenId (struct token* token){ 
    if (token->new_client_request == FALSE){
        if (token->tokenId != current_tokenId){
            printf("Invalid tokenId\n");
            exit(-1);
        }
    }
}

void set_tokenId(struct token* token){ 
    // don't set new token ID if control token
    if (token->new_client_request == FALSE){    
         token->tokenId = get_random_int();
         current_tokenId = token->tokenId;
    }
}

int check_if_new_client(){
    char str[5];
    strcpy(str, "NEW");
    if (strncmp(clientId, str, 3) == 0) return 1;
    return 0;
}


int get_random_int(){
    time_t t;
    srand((unsigned) time(&t));
    return rand()%100;
}

void print_message(char* msg){
    struct token* recv_token = (struct token *) msg;
    printf("FROM: %s MSG: %s\n", recv_token->from, recv_token->message);
}


void write_message(char * msg){    
    struct token* token = (struct token *) msg;
    sprintf(token->message, "%d", get_random_int());
    strcpy(token->from,clientId);
    msg = (char *) token;
   // printf("Message written: %s\n", token->message);
   
}

void write_to_client_message_with_check(char* char_token, int is_your_message, int free_token){
    
    struct token* token = (struct token *) char_token;
    if(!is_your_message && !free_token) {
        if (token->messageId != last_seen_msgID || last_seen_msgID == 0){
            last_seen_msgID = token->messageId;
            return;
        }
        else {   
            make_free(char_token);
            token->messageId++;
            last_seen_msgID = token->messageId;
            return;
        }
        
    }
       
   
    if (last_sender) {
        last_sender = FALSE;
        memset(token->message,0,MAX_MSG_LEN);
        memset(token->to,0,MAX_NAME_LEN);
        memset(token->from,0,MAX_NAME_LEN);
    }
    
    else {       
        if(is_msg_in_buf){
            last_sender=TRUE;
            strcpy(token->message,msg);
            strcpy(token->to,receiver);
            strcpy(token->from,clientId);
            token->messageId++;
            //printf("Message to send To(%s) from(%s)\n",token->to,token->from);
            is_msg_in_buf=FALSE;
        }
        else {
            memset(token->message, 0, MAX_MSG_LEN);
            memset(token->to, 0, MAX_NAME_LEN);
            memset(token->from, 0, MAX_NAME_LEN);
        }
    }
    token->messageId++;
    last_seen_msgID = token->messageId;
}


int f_free_token(char* msg) {
    struct token* recv_token = (struct token *) msg;
    if (strcmp(recv_token->to,"") == 0) return 1;
    return 0;
}

void make_free(char * msg) {
    struct token* token = (struct token *) msg;
    memset(token->message, 0, MAX_MSG_LEN);
    memset(token->to, 0, MAX_NAME_LEN);
    memset(token->from, 0, MAX_NAME_LEN);
}


DWORD WINAPI run_console_thread(){
    
    while(1){
        printf("Do kogo chcesz wyslac wiadmosc?\n");
        gets(receiver);
        printf("Podaj wiadmosc: \n");
        gets(msg);
        is_msg_in_buf = TRUE; 
        printf("your request has been processed target='%s'\n",receiver);
        while(is_msg_in_buf)
        {
            ;
        }
    }
}


