#pragma once

enum class EMessageID : int
{
    UNDEFINED                                = 0,        //��Ȯ��
    C2S_REQ_INSERT_PLAYER                    = 11001,    //Ŭ���̾�Ʈ_���� ȸ������ ��û �޽���
    C2S_REQ_LOGIN_PLAYER                     = 11002,    //Ŭ���̾�Ʈ_���� �α��� ��û �޽���
    C2S_REQ_LOGOUT_PLAYER                    = 11003,    //Ŭ���̾�Ʈ_���� �α׾ƿ� ��û �޽���
    C2S_REQ_INSERT_SESSIONCHATTINGLOG        = 11004,    //Ŭ���̾�Ʈ_���� ����ä�� ��û �޽���
    C2S_REQ_CREATE_SESSION                   = 11005,    //Ŭ���̾�Ʈ_���� ���ǻ��� ��û �޽���
    C2S_REQ_UPDATE_PLAYERSTATE               = 11006,    //Ŭ���̾�Ʈ_���� �÷��̾� ���¼��� ��û �޽���

    S2C_RES_CLINET_CONNECT                   = 20001,    //����_Ŭ���̾�Ʈ ���� ���� �޽���
    S2C_RES_CLINET_DISCONNET                 = 20002,    //����_Ŭ���̾�Ʈ �������� ���� �޽���

    S2C_RES_INSERT_PLAYER                    = 21001,    //����_Ŭ���̾�Ʈ ȸ������ ���� �޽���
    S2C_RES_LOGIN_PLAYER                     = 21002,    //����_Ŭ���̾�Ʈ �α��� ���� �޽���
    S2C_RES_LOGOUT_PLAYER                    = 21003,    //����_Ŭ���̾�Ʈ �α׾ƿ� ���� �޽���
    S2C_RES_INSERT_SESSIONCHATTINGLOG        = 21004,    //����_Ŭ���̾�Ʈ ����ä�� ���� �޽���
    S2C_RES_CREATE_SESSION                   = 21005,    //����_Ŭ���̾�Ʈ ���ǻ��� ���� �޽���
    S2C_RES_UPDATE_PLAYERSTATE               = 21006,    //����_Ŭ���̾�Ʈ �÷��̾� ���¼��� ���� �޽���
};

enum class EProcessFlag : int
{
    UNDEFINED    = 0,  //������
    PROCESS_OK   = 1,  //ó���Ϸ�
    PROCESS_FAIL = 2   //ó������
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
    MessageHeader MsgHead;
    char PLAYER_ID[30];
    char PLAYER_PWD[30];
    char PLAYER_NAME[30];
    int PLAYER_GOLD;
    int PLAYER_EXP;
    int PLAYER_LEVEL;
    int PLAYERCHAR_TYPE;
    int PLAYERCHAR_BODY_SLOT;
    int PLAYERCHAR_HEAD_SLOT;
    int PLAYERCHAR_JUMP_STAT;
    int PLAYERCHAR_STAMINA_STAT;
    int PLAYERCHAR_SPEED_STAT;
};

struct MessageReqLogoutPlayer
{
    MessageHeader MsgHeader;
    char PLAYER_ID[30];
};


struct MessageResLogoutPlayer
{
    MessageHeader MsgHead;
    int PROCESS_FLAG;        
};


struct MessageReqSessionChattingLog
{
    MessageHeader MsgHead;
    char PLAYER_ID[30];
    char PLAYER_NAME[30];
    char CHAT_MSG[100];
};

struct MessageResSessionChattingLog
{
    MessageHeader MsgHead;
    char PLAYER_ID[30];
    char PLAYER_NAME[30];
    char CHAT_MSG[100];
};

struct MessageReqCreateSession
{
    MessageHeader MsgHead;
    char HOST_PLAYER_ID[30];
    char HOST_PLAYER_NAME[30];
    char SESSION_NAME[30];
    char SESSION_PASSWORD[30];
    int SESSION_PLAYER;
};

struct MessageResCreateSession
{
    MessageHeader MsgHead;
    int SESSION_ID;
    char HOST_PLAYER_ID[30];
    char HOST_PLAYER_NAME[30];
    char SESSION_NAME[30];
    char SESSION_PASSWORD[30];
    int SESSION_PLAYER;
    int SESSION_STATE;
};

struct MessageReqUpdatePlayerState
{
    MessageHeader MsgHead;
    char PLAYER_ID[30];
    char PLAYER_NAME[30];
    int PLAYERCHAR_TYPE;
    int PLAYERCHAR_BODY_SLOT;
    int PLAYERCHAR_HEAD_SLOT;
    int PLAYERCHAR_JUMP_STAT;
    int PLAYERCHAR_STAMINA_STAT;
    int PLAYERCHAR_SPEED_STAT;
};

struct MessageResUpdatePlayerState
{
    MessageHeader MsgHead;
    int PROCESS_FLAG;
};