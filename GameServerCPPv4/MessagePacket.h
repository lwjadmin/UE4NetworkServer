#pragma once

enum class EMessageID : int
{
    UNDEFINED                                = 0,        //미확인
    C2S_REQ_INSERT_PLAYER                    = 11001,    //클라이언트_서버 회원가입 요청 메시지
    C2S_REQ_LOGIN_PLAYER                     = 11002,    //클라이언트_서버 로그인 요청 메시지
    C2S_REQ_LOGOUT_PLAYER                    = 11003,    //클라이언트_서버 로그아웃 요청 메시지
    C2S_REQ_INSERT_SESSIONCHATTINGLOG        = 11004,    //클라이언트_서버 세션채팅 요청 메시지
    C2S_REQ_CREATE_SESSION                   = 11005,    //클라이언트_서버 세션생성 요청 메시지
    C2S_REQ_UPDATE_PLAYERSTATE               = 11006,    //클라이언트_서버 플레이어 상태수정 요청 메시지

    S2C_RES_CLINET_CONNECT                   = 20001,    //서버_클라이언트 연결 응답 메시지
    S2C_RES_CLINET_DISCONNET                 = 20002,    //서버_클라이언트 연결해제 응답 메시지

    S2C_RES_INSERT_PLAYER                    = 21001,    //서버_클라이언트 회원가입 응답 메시지
    S2C_RES_LOGIN_PLAYER                     = 21002,    //서버_클라이언트 로그인 응답 메시지
    S2C_RES_LOGOUT_PLAYER                    = 21003,    //서버_클라이언트 로그아웃 응답 메시지
    S2C_RES_INSERT_SESSIONCHATTINGLOG        = 21004,    //서버_클라이언트 세션채팅 응답 메시지
    S2C_RES_CREATE_SESSION                   = 21005,    //서버_클라이언트 세션생성 응답 메시지
    S2C_RES_UPDATE_PLAYERSTATE               = 21006,    //서버_클라이언트 플레이어 상태수정 응답 메시지
};

enum class EProcessFlag : int
{
    UNDEFINED    = 0,  //미정의
    PROCESS_OK   = 1,  //처리완료
    PROCESS_FAIL = 2   //처리실패
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