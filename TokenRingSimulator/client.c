#include "commonFunc.h"
#include "UDPtokenRing.h"
#include "TCPtokenRing.h"




// cmd args:    client ID, client port, next node's IP, next node's port, 0/1 [has_token during start], tcp/udp
// if new client wants to join token ring client ID should be like: NEW_CLIENT ID 
int main(int argc, char *argv[]) {
    
    start_winsock();
     
    if (argc != 7) {
       printf("Correct commend line: clientID, port, neighbour_sock's IP, neighbour_sock's port , has_token during start, tcp/udp");
       return -1;
    }

    clientId = argv[1]; 
    listenerPort = atoi(argv[2]); 
    neighIP = argv[3];
    neighPort = atoi(argv[4]);
    
    startToken = atoi(argv[5]);    
    if (startToken == 1){ spec_node = TRUE; }
    
    protocol = argv[6]; 
    
    create_multicast_socket();
    
    HANDLE consoleThread = CreateThread(NULL, 0, run_console_thread, NULL, 0, NULL);
   
    if (strcmp(protocol, "tcp") == 0) handle_tcp_sock();           
    else if (strcmp(protocol, "udp") == 0) handle_udp_sock();   
    else {
        printf("Incorrect protocol! Type tcp or udp");
        return -1;
    }
}












