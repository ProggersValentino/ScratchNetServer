#pragma once
#include <queue>

#include "Socket.h"

class Server
{
public:
    void MainProcess();
    
private:
    Socket listeningSocket;
    Socket actionSocket;
};
