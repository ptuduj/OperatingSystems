#include "TCPtokenRing.h"


SOCKET senderTCP;
HANDLE sem;



void createSem(){
    sem = CreateSemaphore(NULL, 1, 1, NULL);        
    if (sem == NULL){
        printf("CreateSemaphore error: %d\n", GetLastError());
    }
}

void getSem(){
    if (WaitForSingleObject(sem, INFINITE) != WAIT_OBJECT_0){
        printf("Decreasing sem error: %d\n", GetLastError());
    }
}

void releaseSem(){
    if (!ReleaseSemaphore(sem, 1, NULL) ){
        printf("ReleaseSemaphore error: %d\n", GetLastError());
    }    
}


SOCKET init_socket_tcp(){
  SOCKET sock =  socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  
  if (sock == INVALID_SOCKET){
      printf("Error on create tcp socket\n");
      check_winsock_last_error();
  }
  return sock;
}


void handle_connect(SOCKET sock, char* address, int port){
    
    struct sockaddr_in neighb_address;
    neighb_address.sin_family = AF_INET;
    neighb_address.sin_addr.s_addr = inet_addr(address);
    neighb_address.sin_port = htons(port);
    
    if (connect(sock, (SOCKADDR*) &neighb_address, sizeof(neighb_address)) == SOCKET_ERROR){
        printf("Error on connect tcp socket\n");
        check_winsock_last_error();
    }  
}


DWORD WINAPI run(){  
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(listenerPort + 10);

    if (bind(requets_listener, (SOCKADDR*) &addr, sizeof(addr)) == SOCKET_ERROR){
         printf("RUN: Error on bind tcp socket\n");
         check_winsock_last_error(); 
    }

    if (listen(requets_listener, SOMAXCONN) == SOCKET_ERROR){
         printf("RUN: Error on listen tcp socket\n");
         check_winsock_last_error();       
    }
    
    while (1){
        struct sockaddr_in client_addr;
        int len = sizeof(client_addr);
        SOCKET accept_sock = accept(requets_listener, (SOCKADDR*) &client_addr, &len);

        if (accept_sock == INVALID_SOCKET){
            printf("RUN: Error on accept tcp socket\n");
            check_winsock_last_error();
        }
        getSem();
        
        if (shutdown(senderTCP, SD_BOTH) != 0){
            printf("RUN: Problem during shutdown\n");
            check_winsock_last_error(); 
        }

        if(closesocket(senderTCP) == SOCKET_ERROR)
        {
            printf("RUN: Problem during close\n");
            check_winsock_last_error(); 
        }
        
        // next node's address
        struct sockaddr_in neigbour;
        neigbour.sin_family = AF_INET;
        neigbour.sin_addr.s_addr = inet_addr(neighIP);
        neigbour.sin_port = htons(neighPort);
        
        // send next node address to new client
        if (send(accept_sock, (char*) &neigbour, sizeof(neigbour), 0) == SOCKET_ERROR){
            printf("RUN SEND: Error on send tcp socket\n");
            check_winsock_last_error(); 
        }

        senderTCP = accept_sock;
          
        releaseSem();
    }
}


SOCKET handle_new_clientTCP(SOCKET sock){
      
    // recv next node address from previous node
    struct sockaddr_in nex_node_addr;
    if (recv(sock, (char*) &nex_node_addr, sizeof(nex_node_addr), 0) == SOCKET_ERROR){
            printf("NEW CLIENT RCV: Error on recv tcp socket\n");
            check_winsock_last_error();
    }
    //printf("Recieved and trying to connect to port:%d\n", ntohs(nex_node_addr.sin_port));
    
    // connect to next node
    if (connect(senderTCP, (SOCKADDR*) &nex_node_addr, sizeof(nex_node_addr)) < 0){
        printf("NEW CLIENT: Error on connect tcp socket\n");
        check_winsock_last_error();
    } 
    return sock;
}


int handle_tcp_sock(){ 
    createSem();
  
    senderTCP = init_socket_tcp(); 
    SOCKET listener = init_socket_tcp();   
    requets_listener = init_socket_tcp();
    SOCKET accept_sock;
   
    HANDLE thread = CreateThread(NULL, 0, run, NULL, 0, NULL);
    

    if (spec_node == TRUE){
        handle_connect(senderTCP, neighIP, neighPort);
        
        // create token
        struct token token;           
        token.new_client_request = FALSE;
        set_tokenId(&token); 
        write_message((char*) &token);
        make_free((char*) &token);
        if (send(senderTCP, (char*) &token, sizeof(token), 0) == SOCKET_ERROR) {
             printf("Error on spec_mode send\n");
             check_winsock_last_error();      
        }     
        
        accept_sock = bind_listen_and_accept(listener, "127.0.0.1", listenerPort);      
    }
     
    else if (check_if_new_client() !=1)  {       
        accept_sock = bind_listen_and_accept(listener, "127.0.0.1", listenerPort);
        handle_connect(senderTCP, neighIP, neighPort);
    }
    
    else {
        accept_sock = init_socket_tcp();
        handle_connect(accept_sock, "127.0.0.1", neighPort + 10);   
    
        handle_new_clientTCP(accept_sock);
        struct sockaddr_in my_addr;
        my_addr.sin_family = AF_INET;
        my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        my_addr.sin_port = htons(listenerPort);
        bind(listener,(SOCKADDR*)&my_addr,sizeof(my_addr));
        listen(listener,0);
    }
    
   
    while(1){
        char recvbuf[100];
        struct sockaddr_in senderAddress;
        int iSize= sizeof(senderAddress);
        int ret= recvfrom(accept_sock, recvbuf, sizeof(recvbuf), 0, (SOCKADDR*)&senderAddress, &iSize);
        //if(ret==0) printf("got 0\n");

        while (ret== SOCKET_ERROR || ret==0){
            
            if (WSAGetLastError() == WSAENOTCONN || ret==0)
            {
               // printf("\nGot disconnected \n");
                accept_sock = accept(listener, NULL, NULL);    // accept connection from new node
            }    
            else
            {
                printf("WHILE: Error on recv tcp socket\n");
                check_winsock_last_error();
                exit(-1);
            }
            ret=recvfrom(accept_sock, recvbuf, sizeof(recvbuf), 0,(SOCKADDR*)&senderAddress,&iSize);
        }
        

        Sleep(1000);       
        _log();
               
        int is_your_msg = process_received_token_tcp(recvbuf);
        int is_free= f_free_token(recvbuf);
        write_to_client_message_with_check(recvbuf, is_your_msg, is_free);
        
                
        getSem();
        
        // send message to neighbour_sock         
        if (spec_node == 1) { 
            check_if_valid_tokenId((struct token*) recvbuf);
            set_tokenId((struct token*) recvbuf);            
        }
        
        //write_message(recvbuf);
        //printf("Written Token ID: %d\n", ((struct token*) recvbuf)->tokenId);
        
        if (send(senderTCP, recvbuf, sizeof(recvbuf), 0) == SOCKET_ERROR){
             printf("WHILE: Error on send\n");
             check_winsock_last_error();      
        }   
        releaseSem();        
    } 
}


SOCKET bind_listen_and_accept(SOCKET listener, char* address, int port){
    struct sockaddr_in listener_address;
    listener_address.sin_family = AF_INET;
    listener_address.sin_addr.s_addr = inet_addr(address);
    listener_address.sin_port = htons(port);
   
    if (bind(listener, (SOCKADDR*) &listener_address, sizeof(listener_address)) == SOCKET_ERROR){
         printf("XXX:  Error on bind tcp socket\n");
         check_winsock_last_error(); 
    }
    
    if (listen(listener, SOMAXCONN) == SOCKET_ERROR){
         printf("XXX: Error on listen tcp socket\n");
         check_winsock_last_error();       
    }

   
    SOCKET accept_sock=accept(listener, NULL, NULL);
    
    if (accept_sock == INVALID_SOCKET){
        printf("XXX: Error on accept tcp socket\n");
        check_winsock_last_error();
    }    
    return accept_sock;
}

int process_received_token_tcp(char* msg) {
    if(strcmp(((struct token*)msg)->to, clientId)==0) {
        print_message(msg);
        return 1;
    }
    else
        return 0;
}
