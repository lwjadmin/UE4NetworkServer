#pragma once

struct ClientData
{
    SOCKET SocketID;
    char PlayerID[30];
    char PlayerName[30];
    bool LoginYN;

    ClientData()
    {
        SocketID = 0;
        memset(PlayerID, 0, sizeof(PlayerID));
        memset(PlayerName, 0, sizeof(PlayerName));
        LoginYN = false;
    }

    ClientData(SOCKET InSocketID) : ClientData()
    {
        SocketID = InSocketID;
    }
};