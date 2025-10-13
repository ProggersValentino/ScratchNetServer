#include "Server.h"
#include "Address.h"
#include "ScratchPacketHeader.h"
#include "Payload.h"
#include "PacketSerialization.h"

void Server::MainProcess()
{
    //playerRecord.reserve(maxPlayers); //assign amount of players
    //playerConnected.reserve(maxPlayers); //assign amount of players

    listeningSocket = *RetrieveSocket(); //create new socket
    listeningSocket.OpenSock(30000, true);

    packetAckMaintence = GenerateScratchAck();

    char recieveBuf[30];
    int size = 30;

    while (true)
    {
        Address* address = CreateAddress();
        int recievedBytes = listeningSocket.Receive(*address, recieveBuf, size);

        //have recieved any packets?
        if (recievedBytes > 0)
        {
            unsigned int from_address = ntohl(address->GetAddressFromSockAddrIn());
            unsigned int from_port = ntohs(address->GetPortFromSockAddrIn());

            //find if we need to assign a new client
            ClientRecord* currentClient = nullptr; //creating a new clientRecord on the stack
            int clientConnectionStatus = DetermineClient(address, currentClient);

            //how do we handle the code received
            switch (clientConnectionStatus)
            {
            case -1: //cant connect
                continue;

            case 1:
                Snapshot* initialSnap = CreateEmptySnapShot();
                currentClient->clientSSRecordKeeper->InsertNewRecord(0, *initialSnap);
                break;
            default:
                break;
            }
            

            ScratchPacketHeader recvHeader = *InitEmptyPacketHeader();
            Payload recievedPayload = *CreateEmptyPayload();

            printf("Recieved %d bytes\n", recievedBytes);
            printf("Received package: %s '\n'", recieveBuf);
            printf("from port: %d and ip: %d \n", from_address, from_port);

            DeconstructPacket(recieveBuf, recvHeader, recievedPayload);

            char tempbuf[13] = { 0 };
            int payloadLoco = sizeof(recvHeader);
            memcpy(&tempbuf, &recieveBuf[payloadLoco], 13);

            if (!CompareCRC(recvHeader, tempbuf, 13))
            {
                std::cout << "Failed CRC Check" << std::endl;
                continue;
            }

            std::cout << "CRC Check Succeeded" << std::endl;
            //packet maintence
            packetAckMaintence->InsertRecievedSequenceIntoRecvBuffer(recvHeader.sequence); //insert sender's packet sequence into our local recv sequence buf

            packetAckMaintence->OnPacketAcked(recvHeader.ack); //acknowledge the most recent packet that was recieved by the sender

            packetAckMaintence->AcknowledgeAckbits(recvHeader.ack_bits, recvHeader.ack); //acknowledge the previous 32 packets starting from the most recent acknowledged from the sender

            if (recvHeader.sequence < packetAckMaintence->mostRecentRecievedPacket) //is the packet's sequence we just recieved higher than our most recently recieved packet sequence?
            {
                std::cout << "Packet out of order" << std::endl;
                continue;
            }

            packetAckMaintence->mostRecentRecievedPacket = recvHeader.sequence; //only update the most recent sequence if the recieved one is higher than one the stored
            std::cout << "Packet accepted" << std::endl;
            
            //apply changes to other clients 


            //send echo
            int sendBytes = sendto(listeningSocket.GetSocket(), recieveBuf, size, 0, (SOCKADDR*)&address->sockAddr, sizeof(address->GetSockAddrIn()));

            if (sendBytes == SOCKET_ERROR)
            {
                printf("Error sending data with %d \n", WSAGetLastError());
                continue;
            }
        }
    }
}

int Server::FindPlayer(Address player)
{
    for (int i = 0; i < maxPlayers; i++)
    {
        if (playerConnected[i] && *playerRecord[i].clientAddress == player) 
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
    return *playerRecord[clientIndex].clientAddress;
}

const ClientRecord& Server::GetClientRecord(int clientIndex)
{
    return playerRecord[clientIndex];
}

bool Server::TryToAddPlayer(Address* potentialPlayer, ClientRecord* OUTRecord)
{
    int playerLoco = FindPlayer(*potentialPlayer);

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

    playerRecord[freeSpot] = ClientRecord(potentialPlayer);
    playerConnected[freeSpot] = true;

    if(OUTRecord != NULL)
    {
        OUTRecord = &playerRecord[freeSpot];
    }

    return true;
}

int Server::DetermineClient(Address* clientAddress, ClientRecord* OUTRecord)
{
    ClientRecord newClient = ClientRecord(); //creating a new clientRecord on the stack

    int index = FindPlayer(*clientAddress);

    if (index == -1)
    {
        bool playerAssigned = TryToAddPlayer(clientAddress, OUTRecord);

        if (playerAssigned) //first time connection
        {
            std::cout << "New player assigned and connected :)" << std::endl;
            return 1; 
        }
        else //do not process what this client is trying to send 
        {
            std::cout << "Failed to connect new player :(" << std::endl;
            return -1;
        }

        
    }
    else //long time connected
    {
        *OUTRecord = playerRecord[index];
        return 0;
    }
    
}
