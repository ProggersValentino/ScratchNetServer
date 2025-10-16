#include "Server.h"
#include "Address.h"
#include "ScratchPacketHeader.h"
#include "Payload.h"
#include "PacketSerialization.h"
#include <thread>

void Server::MainProcess()
{
    //playerRecord.reserve(maxPlayers); //assign amount of players
    //playerConnected.reserve(maxPlayers); //assign amount of players

    listeningSocket = *RetrieveSocket(); //create new socket
    listeningSocket.OpenSock(30000, true);

    packetAckMaintence = GenerateScratchAck();

    char recieveBuf[30];
    int size = 30;

    isHeartBeatActive = true;

    std::thread heartBeatWorker(SendHeartBeat); //start heart beat on new thread

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
            currentClient->packetAckMaintence->InsertRecievedSequenceIntoRecvBuffer(recvHeader.sequence); //insert sender's packet sequence into our local recv sequence buf

            currentClient->packetAckMaintence->OnPacketAcked(recvHeader.ack); //acknowledge the most recent packet that was recieved by the sender

            currentClient->packetAckMaintence->AcknowledgeAckbits(recvHeader.ack_bits, recvHeader.ack); //acknowledge the previous 32 packets starting from the most recent acknowledged from the sender

            if (recvHeader.sequence < packetAckMaintence->mostRecentRecievedPacket) //is the packet's sequence we just recieved higher than our most recently recieved packet sequence?
            {
                std::cout << "Packet out of order" << std::endl;
                continue;
            }

            currentClient->packetAckMaintence->mostRecentRecievedPacket = recvHeader.sequence; //only update the most recent sequence if the recieved one is higher than one the stored
            std::cout << "Packet accepted" << std::endl;
            

            //apply changes to other clients 
            
            Snapshot extractedChanges = Snapshot();
            Snapshot newBaseline = Snapshot();


            //update snapshot baseline
            switch (recvHeader.packetCode)
            {
            case 11:
                extractedChanges = DeconstructRelativePayload(recievedPayload);
                newBaseline = ApplyChangesToSnapshot(*currentClient->clientSSRecordKeeper->baselineRecord.recordedSnapshot, extractedChanges);
                currentClient->clientSSRecordKeeper->InsertNewRecord(recvHeader.sequence, newBaseline);
                
                UpdateLocalNetworkedObjectsOnClientRecords(*currentClient, newBaseline); //update all the client record's networkedObject with this change 

                ReplicatedChangeToOtherClients(*currentClient, extractedChanges, 11); //send the change to the other connected clients
                
                break;

            case 12:
                extractedChanges = DeconstructAbsolutePayload(recievedPayload);
                currentClient->clientSSRecordKeeper->InsertNewRecord(recvHeader.sequence, extractedChanges);

                UpdateLocalNetworkedObjectsOnClientRecords(*currentClient, extractedChanges); //update all the client record's networkedObject with this change 
             
                ReplicatedChangeToOtherClients(*currentClient, extractedChanges, 12); //send the change to the other connected clients
                break;
            default:
                break;
            }


           
            

            //send echo
            int sendBytes = sendto(listeningSocket.GetSocket(), recieveBuf, size, 0, (SOCKADDR*)&address->sockAddr, sizeof(address->GetSockAddrIn()));

            if (sendBytes == SOCKET_ERROR)
            {
                printf("Error sending data with %d \n", WSAGetLastError());
                continue;
            }
        }
    }

    heartBeatWorker.join();
    listeningSocket.Close();
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

ClientRecord& Server::GetClientRecord(int clientIndex)
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

bool Server::UpdateClientObjects(ClientRecord* clientToUpdate, Snapshot selectedObjectToUpdate)
{
    if (clientToUpdate == nullptr)
    {
        return false;
    }


    if (!clientToUpdate->TryUpdatingNetworkedObject(selectedObjectToUpdate.objectId, selectedObjectToUpdate))
    {
        clientToUpdate->TryInsertNewNetworkObject(selectedObjectToUpdate.objectId, selectedObjectToUpdate);
    }

    return true;
}

void Server::UpdateLocalNetworkedObjectsOnClientRecords(ClientRecord clientWithUpdates, Snapshot ObjectChanges)
{
    for (int i = 0; i < playerConnected.size(); i++)
    {
        std::vector<char> changedValues; //the changes to the data to be sent over

        std::vector<EVariablesToChange> changedVariables;

        ClientRecord& client = GetClientRecord(i);

        UpdateClientObjects(&client, ObjectChanges);

        
    }
}

void Server::ReplicateChangeGroupToAllClients()
{
    
    for (int i = 0; i < playerConnected.size(); i++)
    {
        if (!playerConnected[i]) //dont need to update a disconnected player
        {
            continue;
        }

       char transmitBuf[30] = { 0 };

        ClientRecord client = GetClientRecord(i);

        for (auto pair = client.networkedObjects.begin(); pair != client.networkedObjects.end(); ++pair)
        {
            ScratchPacketHeader heartBeatHeader = ScratchPacketHeader(12, client.clientSSRecordKeeper->baselineRecord.packetSequence, client.packetAckMaintence->currentPacketSequence,
                client.packetAckMaintence->mostRecentRecievedPacket, client.packetAckMaintence->GetAckBits(client.packetAckMaintence->mostRecentRecievedPacket));

            Payload heartBeatPayload = ConstructAbsolutePayload(pair->second); //grabbing the value 

            ConstructPacket(heartBeatHeader, heartBeatPayload, transmitBuf);

            int sentBytes = listeningSocket.Send(*client.clientAddress, transmitBuf, 30);
        }

    }
}

void Server::ReplicateChangeToAllClients(Snapshot changes)
{
    for (int i = 0; i < playerConnected.size(); i++)
    {
        if (!playerConnected[i]) //dont need to update a disconnected player
        {
            continue;
        }

       char transmitBuf[30] = { 0 };

        ClientRecord& client = GetClientRecord(i);

        ScratchPacketHeader heartBeatHeader = ScratchPacketHeader(12, client.clientSSRecordKeeper->baselineRecord.packetSequence, client.packetAckMaintence->currentPacketSequence,
            client.packetAckMaintence->mostRecentRecievedPacket, client.packetAckMaintence->GetAckBits(client.packetAckMaintence->mostRecentRecievedPacket));

        Payload heartBeatPayload = ConstructAbsolutePayload(changes); //grabbing the value 

        ConstructPacket(heartBeatHeader, heartBeatPayload, transmitBuf);

        int sentBytes = listeningSocket.Send(*client.clientAddress, transmitBuf, 30);

    }
}

void Server::ReplicatedChangeToOtherClients(ClientRecord ClientSentChanges, Snapshot changes, int packetCode)
{
    for (int i = 0; i < playerConnected.size(); i++)
    {
        if (!playerConnected[i]) //dont need to update a disconnected player
        {
            continue;
        }

        char transmitBuf[30] = { 0 };

        ClientRecord& client = GetClientRecord(i);

        if (client == ClientSentChanges)
        {
            continue;
        }

        ScratchPacketHeader heartBeatHeader = ScratchPacketHeader(packetCode, client.clientSSRecordKeeper->baselineRecord.packetSequence, client.packetAckMaintence->currentPacketSequence,
            client.packetAckMaintence->mostRecentRecievedPacket, client.packetAckMaintence->GetAckBits(client.packetAckMaintence->mostRecentRecievedPacket));

        Payload heartBeatPayload = ConstructAbsolutePayload(changes); //grabbing the value 

        ConstructPacket(heartBeatHeader, heartBeatPayload, transmitBuf);

        int sentBytes = listeningSocket.Send(*client.clientAddress, transmitBuf, 30);

    }
}

void Server::RelayClientPosition(ClientRecord client)
{
    char transmitBuf[30] = { 0 };


    ScratchPacketHeader heartBeatHeader = ScratchPacketHeader(22, client.clientSSRecordKeeper->baselineRecord.packetSequence, client.packetAckMaintence->currentPacketSequence,
        client.packetAckMaintence->mostRecentRecievedPacket, client.packetAckMaintence->GetAckBits(client.packetAckMaintence->mostRecentRecievedPacket)); //sending a packet for the purpose of updating ACK

    Payload heartBeatPayload = ConstructAbsolutePayload(*client.clientSSRecordKeeper->baselineRecord.recordedSnapshot); //grabbing the baseline record to send to client

    ConstructPacket(heartBeatHeader, heartBeatPayload, transmitBuf);

    int sentBytes = listeningSocket.Send(*client.clientAddress, transmitBuf, 30);
}


void Server::SendHeartBeat()
{
    while (isHeartBeatActive)
    {
        for (int i = 0; i < playerConnected.size(); i++)
        {
            if (!playerConnected[i]) //dont need to update a disconnected player
            {
                continue;
            }

            char transmitBuf[30] = { 0 };

            ClientRecord client = GetClientRecord(i); //grabbing a copy of the client record to prevent it from getting entangled with the main thread 

            RelayClientPosition(client); //update the baseline for the client 
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

}
