#ifndef COMMONFUNC_H_INCLUDED
#define COMMONFUNC_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <Winsock2.h>
#include <Windows.h>
#include <time.h>

# define TRUE 1
# define FALSE 0
# define MAX_NAME_LEN 16
# define MAX_MSG_LEN 30 

struct token{    
   int tokenId;
   int new_client_request;          // used for UDP communication
   char from[MAX_NAME_LEN+1];
   char to[MAX_NAME_LEN+1];        
   char message[MAX_MSG_LEN+1];
   int messageId;
};


extern SOCKET multicast_sock;
extern struct sockaddr_in multicast_addr;

extern SOCKET requets_listener;
extern int spec_node;
extern int current_tokenId;

extern char* clientId;
extern int listenerPort;
extern char* neighIP;
extern int neighPort;
extern int startToken;
extern char* protocol;

extern char receiver[MAX_NAME_LEN];
extern char msg[MAX_MSG_LEN];
extern int is_msg_in_buf;
extern int last_seen_msgID;


void check_winsock_last_error();
void start_winsock();////////////
void create_multicast_socket();
void _log();
void check_if_valid_tokenId (struct token* token);
void set_tokenId(struct token* token);
int check_if_new_client();
int get_random_int();
void print_message(char* msg);
void write_message(char * msg);
SOCKET init_socket_udp();
void write_to_client_message_with_check(char* char_token,int is_your_message,int free_token);
int f_free_token(char* msg);
void make_free(char * msg);
DWORD WINAPI run_console_thread();



#endif  // COMMONFUNC_H_INCLUDED







