#pragma once
#include "Socket.h"
#include "ScratchAck.h"
#include "ClientRecord.h"

#include <vector>
#include <array>
#include <queue>
#include <unordered_map>

const int maxPlayers = 4;

class Server
{
public:
    void MainProcess();

    //returns the position the player is located otherwise -1
    int FindPlayer(Address player);
    int FindFreeClientIndex() const;
    bool IsClientConnected(int clientIndex);
    const Address& GetClientAddress(int clientIndex);
    ClientRecord& GetClientRecord(int clientIndex);

    bool TryToAddPlayer(Address* potentialPlayer, ClientRecord* OUTRecord = nullptr);

    /// <summary>
    /// will return a code based of the connection status of the player 
    /// </summary>
    /// <param name="clientAddress"></param>
    /// <param name="OUTRecord"></param>
    /// <returns>returns either 0, 1, -1 
    /// 0 -> Player has been connected for a while
    /// 1 -> New player is connecting
    /// -1 -> Can't connect player to lobby
    /// </returns>
    int DetermineClient(Address* clientAddress, ClientRecord* OUTRecord);

    
    bool UpdateClientObjects(ClientRecord* clientToUpdate, Snapshot selectedObjectToUpdate);

    //to update all the connected clients to the current state of the client that sent a packet
    void UpdateLocalNetworkedObjectsOnClientRecords(ClientRecord clientWithUpdates, Snapshot ObjectChanges);

    //sends changes to all clients connected
    void ReplicateChangeGroupToAllClients();

    void ReplicateChangeToAllClients(Snapshot changes);

    //used if we have a main sender and only need to update the other clients connected
    void ReplicatedChangeToOtherClients(ClientRecord ClientSentChanges, Snapshot changes, int packetCode);

    //to synchronize the given client's position 
    void RelayClientPosition(ClientRecord client);

    //to send each client a packet to update the baseline under packet code 22 every 250ms 
    void SendHeartBeat();

    
private:
    Socket listeningSocket;
    Socket actionSocket;

    ScratchAck* packetAckMaintence;

    std::unordered_map<int, Snapshot> networkedObjects;
    
    int numOfConnectedClients;

    std::array<bool, maxPlayers> playerConnected;
    std::array<ClientRecord, maxPlayers> playerRecord;

    bool isHeartBeatActive = false;
};
