#include "Server.h"
#include "Address.h"

#pragma comment(lib, "zlib.lib")

void Server::MainProcess()
{
    playerAddress.reserve(maxPlayers); //assign amount of players
    playerConnected.reserve(maxPlayers); //assign amount of players

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

int Server::FindPlayer(Address player)
{
    for (int i = 0; i < maxPlayers; i++)
    {
        if (playerConnected[i] && playerAddress[i] == player) 
        {
            return i;
        }
    }

    return -1;
}

int Server::FindFreeClientIndex() const
{
    for (int i = 0; i < maxPlayers; i++)
    {
        if (!playerConnected[i]) //if there is a free spot the playerconnected array will have a spot that is set to false
        {
            return i;
        }
    }
    return -1;
}

bool Server::IsClientConnected(int clientIndex)
{
    return playerConnected[clientIndex];
}

const Address& Server::GetClientAddress(int clientIndex)
{
    return playerAddress[clientIndex];
}

bool Server::TryToAddPlayer(Address potentialPlayer)
{
    int playerLoco = FindPlayer(potentialPlayer);

    if (playerLoco > 0 && IsClientConnected(playerLoco)) //player is already in 
    {
        std::cout << "ERROR: Player Already Connected! \n" << std::endl;
        return false;
    }

    int freeSpot = FindFreeClientIndex();

    if (freeSpot < 0) //lobby full 
    {
        std::cout << "ERROR: Lobby is full! \n" << std::endl;
        return false;
    }

    playerAddress[freeSpot] = potentialPlayer;
    playerConnected[freeSpot] = true;

    return true;
}
