#include "UDPtokenRing.h"


int handle_udp_sock(){  

    UDPsocket = init_socket_udp();
       
    // bind to your IP address and port
    struct sockaddr_in listener_address;
    listener_address.sin_family = AF_INET;
    listener_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    listener_address.sin_port = htons(listenerPort);
   
    bind(UDPsocket, (SOCKADDR*) &listener_address, sizeof(listener_address)); 
 
    // neighbour's address
    struct sockaddr_in neighb_address;
    neighb_address.sin_family = AF_INET;
    neighb_address.sin_addr.s_addr = inet_addr(neighIP);
    neighb_address.sin_port = htons(neighPort);
   
     // NEW CLIENT 
    if (check_if_new_client() ==1)  {
        struct sockaddr_in*  tmp = handle_new_client(&neighb_address);
        neighb_address.sin_port = tmp->sin_port; 
        neighb_address.sin_addr = tmp->sin_addr;
    }
  
    if (spec_node == TRUE){
        struct token token;           //   create token
        token.new_client_request = FALSE;
        set_tokenId(&token); 
        make_free((char*) &token);
        if (sendto(UDPsocket, (char*) &token, sizeof(token), 0, (SOCKADDR*) &neighb_address, sizeof(neighb_address)) == SOCKET_ERROR) {
             printf("Error on spec_node send\n");
             check_winsock_last_error();      
             return -1;
        }     
    }
    
    
    while(1){
        char recvbuf[100];     
        struct sockaddr_in from;
        int len = sizeof(from);
        
        if (recvfrom(UDPsocket, recvbuf, sizeof(recvbuf), 0, (SOCKADDR*) &from, &len) == SOCKET_ERROR){
            printf("Error on recvfrom udp\n");
            check_winsock_last_error();
        }

        int is_your_msg = process_received_token(&from, &neighb_address, recvbuf);
        int is_free= f_free_token(recvbuf);
        
        Sleep(1000);
        
        _log();
        
        struct token* token = (struct token *) recvbuf;
        
        
        if (token->new_client_request == FALSE){   // to make sure that control token won't be sent after new connection was established   
           
           if (spec_node == TRUE) { 
                check_if_valid_tokenId((struct token*) recvbuf);
                set_tokenId((struct token*) recvbuf);               
            }
            
            write_to_client_message_with_check(recvbuf, is_your_msg, is_free);
            if (sendto(UDPsocket, recvbuf, sizeof(recvbuf),0, (SOCKADDR*) &neighb_address, sizeof(neighb_address)) == SOCKET_ERROR){
                printf("Error on sendto udp\n");
                check_winsock_last_error();
            }
        }  
    }    
}


struct sockaddr_in*  handle_new_client(struct sockaddr_in* neighb_address){
    
    // control token creation
    struct token control_token; 
    control_token.new_client_request = TRUE;
    
    struct sockaddr_in* prev_neighb_address = neighb_address;
    
    
    // send control token to node before you    
    if (sendto(UDPsocket, (char*) &control_token, sizeof(control_token), 0, (SOCKADDR*) neighb_address, sizeof(struct sockaddr_in)) == SOCKET_ERROR){
        printf("Error while sending control token");
        check_winsock_last_error();
    }    
        
 
    // change your next node addreess 
    char buf[30];
    if (recvfrom(UDPsocket, (char*) buf, sizeof(buf), 0, NULL, NULL) == SOCKET_ERROR){
        printf("Error while receiving next node address\n");
        check_winsock_last_error();
    }
    neighb_address = (struct sockaddr_in*) buf;
    
    char ack[20];
    
    if (sendto(UDPsocket, ack, sizeof(ack), 0, (SOCKADDR*) prev_neighb_address, sizeof(struct sockaddr_in)) == SOCKET_ERROR){
        printf("Error while sending ACK\n");
        check_winsock_last_error();
    } 
    return neighb_address;
    
}


int process_received_token(struct sockaddr_in* receiver_addr, struct sockaddr_in* neighb_addr, char* msg){

    struct token* recv_token = (struct token *) msg;
    
    // IF NEW CLIENT WANTS TO JOIN TOKEN RING
    if (recv_token-> new_client_request == TRUE ){
           
        // send your neighbour address to new client
        if (sendto(UDPsocket, (char*) neighb_addr, sizeof(*neighb_addr), 0, (SOCKADDR*) receiver_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR){
            printf("Error on sending next node address to new client\n");
            check_winsock_last_error();
        }
               
        // set your new neigbour
        *neighb_addr = *receiver_addr;       
              
        char recvbuf[100];
        
        // wait for ACK from new client  (to give time new client to build connection)
        if (recvfrom(UDPsocket, recvbuf, sizeof(recvbuf), 0, NULL, NULL) == SOCKET_ERROR){
            printf("Error on receiving ACK from new client");
            check_winsock_last_error();
        }                         
    }
    else {
        //check_if_you_are_receiver
        if (strcmp(recv_token->to, clientId) == 0){
            print_message(msg);
            return 1;
        }
        return 0;
    }
    return 0;
}

