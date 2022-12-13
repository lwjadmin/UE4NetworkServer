#pragma once

enum class EMessageID : int
{
    UNDEFINED = 0,
    S2C_RES_CLINET_CONNECT = 1,
    S2C_RES_CLINET_DISCONNET = 2,
    //
    C2S_REQ_INSERT_PLAYER = 10001,
    S2C_RES_INSERT_PLAYER = 20001,
    //
    C2S_REQ_LOGIN_PLAYER = 10002,
    S2C_RES_LOGIN_PLAYER = 20002,
};

struct MessageHeader
{
    int MessageID;
    int MessageSize;
    int SenderSocketID;
    int ReceiverSocketID;
};

struct MessageReqInsertPlayer
{
    MessageHeader MsgHead;
    char PLAYER_ID[30];
    char PLAYER_PWD[30];
    char PLAYER_NAME[30];
};

struct MessageResInsertPlayer
{
    MessageHeader MsgHead;
    char PLAYER_ID[30];
};


struct MessageReqLoginPlayer
{
    MessageHeader MsgHeader;
    char PLAYER_ID[30];
    char PLAYER_PWD[30];
};

struct MessageResLoginPlayer
{
    MessageHeader MsgHeader;
    char PLAYER_ID[30];
    char PLAYER_PWD[30];
    char PLAYER_NAME[30];


};