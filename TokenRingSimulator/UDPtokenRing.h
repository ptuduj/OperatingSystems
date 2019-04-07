#ifndef UDPTOKENRING_H_INCLUDED
#define UDPTOKENRING_H_INCLUDED

#include "commonFunc.h"

SOCKET UDPsocket;
int handle_udp_sock();
struct sockaddr_in*  handle_new_client(struct sockaddr_in* neighb_address);
int process_received_token(struct sockaddr_in* receiver_addr, struct sockaddr_in* neighb_addr, char* msg);

#endif // UDPTOKENRING_H_INCLUDED