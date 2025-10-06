#pragma once
#include "Socket.h"
#include "ScratchAck.h"

#include <vector>
#include <queue>
#include <unordered_map>

class Server
{
public:
    void MainProcess();

    //returns the position the player is located otherwise -1
    int FindPlayer(Address player);
    int FindFreeClientIndex() const;
    bool IsClientConnected(int clientIndex);
    const Address& GetClientAddress(int clientIndex);

    bool TryToAddPlayer(Address potentialPlayer);
    
private:
    Socket listeningSocket;
    Socket actionSocket;

    ScratchAck* packetAckMaintence;

    int maxPlayers = 4;
    int numOfConnectedClients;

    std::vector<bool> playerConnected;
    std::vector<Address> playerAddress;
};
