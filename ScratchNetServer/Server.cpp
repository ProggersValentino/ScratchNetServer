#include "Server.h"
#include "Address.h"
#include "ScratchPacketHeader.h"
#include "Payload.h"
#include "PacketSerialization.h"

void Server::MainProcess()
{
    //playerAddress.reserve(maxPlayers); //assign amount of players
    //playerConnected.reserve(maxPlayers); //assign amount of players

    listeningSocket = *RetrieveSocket(); //create new socket
    listeningSocket.OpenSock(30000, true);

    packetAckMaintence = GenerateScratchAck();

    char recieveBuf[40];
    int size = 40;

    while (true)
    {
        Address* address = CreateAddress();
        int recievedBytes = listeningSocket.Receive(*address, recieveBuf, size);

        //have recieved any packets?
        if (recievedBytes > 0)
        {
            unsigned int from_address = address->GetAddressFromSockAddrIn();
            unsigned int from_port = address->GetPortFromSockAddrIn();

            ScratchPacketHeader* recvHeader = InitEmptyPacketHeader();
            Payload recievedPayload = *CreateEmptyPayload();

            printf("Recieved %d bytes\n", recievedBytes);
            printf("Received package: %s '\n'", recieveBuf);
            printf("from port: %d and ip: %d \n", from_address, from_port);

            DeconstructPacket(recieveBuf, *recvHeader, recievedPayload);

            char tempbuf[15] = { 0 };
            SerializePayload(recievedPayload, tempbuf);

            /*if (!CompareCRC(*recvHeader, tempbuf))
            {
                std::cout << "Failed CRC Check" << std::endl;
                return;
            }*/

            std::cout << "CRC Check Succeeded" << std::endl;
            //packet maintence
            packetAckMaintence->InsertRecievedSequenceIntoRecvBuffer(recvHeader->sequence); //insert sender's packet sequence into our local recv sequence buf

            packetAckMaintence->OnPacketAcked(recvHeader->ack); //acknowledge the most recent packet that was recieved by the sender

            packetAckMaintence->AcknowledgeAckbits(recvHeader->ack_bits, recvHeader->ack); //acknowledge the previous 32 packets starting from the most recent acknowledged from the sender

            if (recvHeader->sequence < packetAckMaintence->mostRecentRecievedPacket) //is the packet's sequence we just recieved higher than our most recently recieved packet sequence?
            {
                packetAckMaintence->mostRecentRecievedPacket = recvHeader->sequence;
                std::cout << "Packet out of order" << std::endl;
                return;
            }

            std::cout << "Packet accepted" << std::endl;
            
            //apply changes to other clients 


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
