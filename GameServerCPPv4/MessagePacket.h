#pragma once

enum class EMessageID : int
{
    UNDEFINED = 0,
    S2C_RES_CLINET_CONNECT = 1,
    S2C_RES_CLINET_DISCONNET = 2,
    C2S_REQ_CLIENT_LOGIN = 10001,
    S2C_RES_CLIENT_LOGIN = 20001,
};

struct MessageHeader
{
    int MessageID;
    int MessageSize;
    int SenderSocketID;
    int ReceiverSocketID;
};

struct MessageReqClientLogin
{
    MessageHeader MsgHeader;
    char PLAYER_ID[30];
    char PLAYER_PWD[30];
};

struct MessageResClientLogin
{
    MessageHeader MsgHeader;
    char PLAYER_NAME[30];

};