#ifndef TCPTOKENRING_H_INCLUDED
#define TCPTOKENRING_H_INCLUDED

#include "commonFunc.h"


void createSem();   
void getSem();
void releaseSem();

SOCKET init_socket_tcp();
int handle_tcp_sock();

SOCKET bind_listen_and_accept(SOCKET listener, char* address, int port);
void handle_connect(SOCKET sender, char* address, int port);
DWORD WINAPI run();
SOCKET handle_new_clientTCP(SOCKET tmp_sock);
int process_received_token_tcp(char* msg);

#endif // TCPTOKENRING_H_INCLUDED