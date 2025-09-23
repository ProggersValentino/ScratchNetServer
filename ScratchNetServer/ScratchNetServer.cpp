
#include "NetSockets.h"
#include "Server.h"

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>


// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

int main(int argc, char* argv[])
{
   

    InitializeSockets(); //init sockets

    Server server;
    
    
    printf("starting main process");
    server.MainProcess();

    /*printf("receiving data from socket\n");*/
    
    //recieve data from sendto function

        
    WSACleanup();
    return 0;
}
