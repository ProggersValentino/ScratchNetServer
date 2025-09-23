#include "Server.h"
#include "Address.h"

void Server::MainProcess()
{
    

    listeningSocket = *RetrieveSocket(); //create new socket
    listeningSocket.OpenSock(30000, true);

    char recieveBuf[13];
    int size = 13;

    while (true)
    {
        Address* address = CreateAddress();
        int recievedBytes = listeningSocket.Receive(*address, recieveBuf, size);

        //have recieved any packets?
        if (recievedBytes > 0)
        {
            unsigned int from_address = address->GetAddressFromSockAddrIn();
            unsigned int from_port = address->GetPortFromSockAddrIn();

            printf("Recieved %d bytes\n", recievedBytes);
            printf("Received package: %s '\n'", recieveBuf);
            printf("from port: %d and ip: %d \n", from_address, from_port);

            //send echo
            int sendBytes = sendto(listeningSocket.GetSocket(), recieveBuf, size, 0, (SOCKADDR*)&address->sockAddr, sizeof(address->GetSockAddrIn()));

            if (sendBytes == SOCKET_ERROR)
            {
                printf("Error sending data with %d \n", WSAGetLastError());
                return;
            }
        }
    }
}
