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
    const ClientRecord& GetClientRecord(int clientIndex);

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

    
private:
    Socket listeningSocket;
    Socket actionSocket;

    ScratchAck* packetAckMaintence;

    
    int numOfConnectedClients;

    std::array<bool, maxPlayers> playerConnected;
    std::array<ClientRecord, maxPlayers> playerRecord;
};
