//--Define--------------------------------------------------------------------
//#define _MICRO_TEST_ENABLED
//#define _RUN_AS_LOCALHOST
#define _WINSOCK_DEPRECATED_NO_WARNINGS
//--C Header------------------------------------------------------------------
#include <WinSock2.h> 
#include <Windows.h>
#include <process.h>
//--C++ Header----------------------------------------------------------------
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <string>
//--Custom Header-------------------------------------------------------------
#include "StringHelper.h"
#include "MessagePacket.h"
#include "ClientData.h"
//--Plugin Header-------------------------------------------------------------
#include "jdbc/mysql_connection.h"
#include "jdbc/cppconn/driver.h"
#include "jdbc/cppconn/exception.h"
#include "jdbc/cppconn/prepared_statement.h"
//--Plugin Library------------------------------------------------------------
#pragma comment (lib, "WS2_32.lib")
#pragma comment (lib, "debug/mysqlcppconn.lib")
//--Network-------------------------------------------------------------------
WSAData                      NET_WSADATA = { 0, };
SOCKET                       NET_SERVERSOCKET = NULL;
SOCKADDR_IN                  NET_SERVERADDR = { 0, };
#ifdef _RUN_AS_LOCALHOST
const char* NET_SERVER_IPV4 = "127.0.0.1";
#else
const char* NET_SERVER_IPV4 = "172.16.2.146";
#endif 
const int                    NET_SERVER_PORT = 5001;
const int                    NET_PACKET_SIZE = 512;
std::map<SOCKET, ClientData> CLIENT_POOL;
CRITICAL_SECTION             CS_NETWORK_HANDLER;
//--Thread--------------------------------------------------------------------       
std::map<SOCKET, HANDLE>     THREAD_POOL;
CRITICAL_SECTION             CS_THREAD_HANDLER;
//--DBMS----------------------------------------------------------------------
#ifdef _RUN_AS_LOCALHOST
const std::string            DB_SERVERNAME = "tcp://127.0.0.1:3306";
const std::string            DB_USERNAME = "root";
const std::string            DB_PASSWORD = "Passw0rd";
#else
const std::string            DB_SERVERNAME = "tcp://172.16.2.146:3306";
const std::string            DB_USERNAME = "AnimalGuysAdmin";
const std::string            DB_PASSWORD = "Passw0rd";
#endif 
const std::string            DB_DBNAME = "ANIMALGUYS";
sql::Driver*                 DB_DRIVER = nullptr;
sql::Connection*             DB_CONN = nullptr;
sql::Statement*              DB_STMT = nullptr;
sql::PreparedStatement*      DB_PSTMT = nullptr;
sql::ResultSet*              DB_RS = nullptr;
CRITICAL_SECTION             CS_DB_HANDLER;
//--MainProgram---------------------------------------------------------------
bool                         G_PROGRAMRUNNING = true;

using namespace std;

int ProcessPacket(SOCKET ClientSocket, char* buffer)
{
    int retval = 1;
    string sqlQuery = "";
    int updatedRows = 0;

    char Packet[NET_PACKET_SIZE] = { 0, };
    memcpy(Packet, buffer, sizeof(Packet));
    MessageHeader MsgHead = { 0, };
    memcpy(&MsgHead, Packet, sizeof(MsgHead));

    switch ((EMessageID)MsgHead.MessageID)
    {
        case EMessageID::C2S_REQ_INSERT_PLAYER:
        {
            MessageReqInsertPlayer ReqMsg = { 0, };
            MessageResInsertPlayer ResMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqInsertPlayer));

            EnterCriticalSection(&CS_DB_HANDLER);
            try
            {
                //플레이어 테이블에 해당 ID가 있는지 조회
                sqlQuery = "SELECT 1 FROM PLAYER WHERE PLAYER_ID = ? LIMIT 1";
                DB_PSTMT = DB_CONN->prepareStatement(sqlQuery);
                DB_PSTMT->setString(1, ReqMsg.PLAYER_ID);
                DB_RS = DB_PSTMT->executeQuery();

                if (DB_RS == nullptr || (DB_RS != nullptr && DB_RS->next() == false))
                {
                    //해당 ID가 없으면 INSERT (없으면 INSERT)
                    sqlQuery = "INSERT INTO PLAYER(PLAYER_ID,PLAYER_PWD,PLAYER_NAME)VALUES(?,?,?)";
                    DB_PSTMT = DB_CONN->prepareStatement(sqlQuery);
                    DB_PSTMT->setString(1, ReqMsg.PLAYER_ID);
                    DB_PSTMT->setString(2, ReqMsg.PLAYER_PWD);
                    DB_PSTMT->setString(3, ReqMsg.PLAYER_NAME);
                    updatedRows += DB_PSTMT->executeUpdate();

                    //플레이어 스탯도 초기화해준다!
                    sqlQuery = "INSERT INTO PLAYERSTATE(         \
                                    PLAYER_ID,                   \
                                    PLAYER_GOLD,                 \
                                    PLAYER_EXP,                  \
                                    PLAYER_LEVEL,                \
                                    PLAYERCHAR_TYPE,             \
                                    PLAYERCHAR_BODY_SLOT,        \
                                    PLAYERCHAR_HEAD_SLOT,        \
                                    PLAYERCHAR_JUMP_STAT,        \
                                    PLAYERCHAR_STAMINA_STAT,     \
                                    PLAYERCHAR_SPEED_STAT,       \
                                    REGISTER_DTTM                \
                                )VALUES(                         \
                                    ?,                           \
                                    0,                           \
                                    0,                           \
                                    0,                           \
                                    0,                           \
                                    0,                           \
                                    0,                           \
                                    0,                           \
                                    0,                           \
                                    0,                           \
                                    NOW()                        \
                                )";
                    sqlQuery = string_ReplaceNSpaceTo1Space(sqlQuery);
                    DB_PSTMT = DB_CONN->prepareStatement(sqlQuery);
                    DB_PSTMT->setString(1, ReqMsg.PLAYER_ID);
                    updatedRows += DB_PSTMT->executeUpdate();
                }
            }
            catch (sql::SQLException ex)
            {
                std::cout << "[ERR] SQL Error On C2S_REQ_INSERT_PLAYER. ErrorMsg : " << ex.what() << std::endl;
            }
            if (DB_RS) { DB_RS->close(); DB_RS = nullptr; }
            if (DB_PSTMT) { DB_PSTMT->close(); DB_PSTMT = nullptr; }
            LeaveCriticalSection(&CS_DB_HANDLER);
            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_INSERT_PLAYER;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.ReceiverSocketID = (int)ClientSocket;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResInsertPlayer);
            ResMsg.PROCESS_FLAG = updatedRows >= 2 ? (int)EProcessFlag::PROCESS_OK : (int)EProcessFlag::PROCESS_FAIL;
            retval = send(ClientSocket, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            break;
        }
        case EMessageID::C2S_REQ_LOGIN_PLAYER:
        {
            MessageReqLoginPlayer ReqMsg = { 0, };
            MessageResLoginPlayer ResMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqLoginPlayer));
            bool bLoginSuccess = false;

            EnterCriticalSection(&CS_DB_HANDLER);
            try
            {
                //플레이어 계정정보와 스탯정보 조회
                sqlQuery = "SELECT                               \
                                PLA.PLAYER_ID,                   \
                                PLA.PLAYER_PWD,                  \
                                PLA.PLAYER_NAME,                 \
                                PLS.PLAYER_GOLD,                 \
                                PLS.PLAYER_EXP,                  \
                                PLS.PLAYER_LEVEL,                \
                                PLS.PLAYERCHAR_TYPE,             \
                                PLS.PLAYERCHAR_BODY_SLOT,        \
                                PLS.PLAYERCHAR_HEAD_SLOT,        \
                                PLS.PLAYERCHAR_JUMP_STAT,        \
                                PLS.PLAYERCHAR_STAMINA_STAT,     \
                                PLS.PLAYERCHAR_SPEED_STAT        \
                            FROM PLAYER AS PLA                   \
                                INNER JOIN PLAYERSTATE AS PLS    \
                                ON PLA.PLAYER_ID = PLS.PLAYER_ID \
                            WHERE                                \
                                PLA.PLAYER_ID  = ? AND           \
                                PLA.PLAYER_PWD = ?               \
                            LIMIT 1";
                sqlQuery = string_ReplaceNSpaceTo1Space(sqlQuery);
                DB_PSTMT = DB_CONN->prepareStatement(sqlQuery);
                DB_PSTMT->setString(1, ReqMsg.PLAYER_ID);
                DB_PSTMT->setString(2, ReqMsg.PLAYER_PWD);
                DB_RS = DB_PSTMT->executeQuery();

                if (DB_RS != nullptr && DB_RS->next() == true)
                {
                    //해당 ID가 존재하고, 스탯정보도 존재할 경우 
                    string rsPlayerID = DB_RS->getString("PLAYER_ID").asStdString();
                    memcpy(ResMsg.PLAYER_ID, rsPlayerID.c_str(), rsPlayerID.length());
                    //memcpy(ResMsgForOthers.PLAYER_ID, rsPlayerID.c_str(), rsPlayerID.length());

                    string rsPlayerPWD = DB_RS->getString("PLAYER_PWD").asStdString();
                    memcpy(ResMsg.PLAYER_PWD, rsPlayerPWD.c_str(), rsPlayerPWD.length());

                    string rsPlayerName = DB_RS->getString("PLAYER_NAME").asStdString();
                    memcpy(ResMsg.PLAYER_NAME, rsPlayerName.c_str(), rsPlayerName.length());
                    //memcpy(ResMsgForOthers.PLAYER_NAME, rsPlayerName.c_str(), rsPlayerName.length());

                    ResMsg.PLAYER_GOLD = DB_RS->getInt("PLAYER_GOLD");
                    ResMsg.PLAYER_EXP = DB_RS->getInt("PLAYER_EXP");
                    ResMsg.PLAYER_LEVEL = DB_RS->getInt("PLAYER_LEVEL");
                    ResMsg.PLAYERCHAR_TYPE = DB_RS->getInt("PLAYERCHAR_TYPE");
                    ResMsg.PLAYERCHAR_BODY_SLOT = DB_RS->getInt("PLAYERCHAR_BODY_SLOT");
                    ResMsg.PLAYERCHAR_HEAD_SLOT = DB_RS->getInt("PLAYERCHAR_HEAD_SLOT");
                    ResMsg.PLAYERCHAR_JUMP_STAT = DB_RS->getInt("PLAYERCHAR_JUMP_STAT");
                    ResMsg.PLAYERCHAR_STAMINA_STAT = DB_RS->getInt("PLAYERCHAR_STAMINA_STAT");
                    ResMsg.PLAYERCHAR_SPEED_STAT = DB_RS->getInt("PLAYERCHAR_SPEED_STAT");

                    //현재 접속중인 플레이어 테이블에 INSERT!
                    sqlQuery = "INSERT INTO CURLOGINPLAYER(LOGIN_PLAYER_ID,LOGIN_DTTM,LOGOUT_DTTM,LOGIN_STAT)VALUES(?,NOW(),NULL,1)";
                    DB_PSTMT = DB_CONN->prepareStatement(sqlQuery);
                    DB_PSTMT->setString(1, string(ReqMsg.PLAYER_ID));
                    updatedRows += DB_PSTMT->executeUpdate();

                    //서버에도 해당정보를 저장!
                    EnterCriticalSection(&CS_NETWORK_HANDLER);
                    //UTF-8 String
                    memcpy(CLIENT_POOL[ClientSocket].PlayerID, ResMsg.PLAYER_ID, sizeof(ResMsg.PLAYER_ID));
                    //UTF-8 String
                    memcpy(CLIENT_POOL[ClientSocket].PlayerName, ResMsg.PLAYER_NAME, sizeof(ResMsg.PLAYER_NAME));
                    CLIENT_POOL[ClientSocket].LoginYN = bLoginSuccess = true;
                    LeaveCriticalSection(&CS_NETWORK_HANDLER);
                }
            }
            catch (sql::SQLException ex)
            {
                std::cout << "[ERR] SQL Error On C2S_REQ_LOGIN_PLAYER. ErrorMsg : " << ex.what() << std::endl;
            }
            if (DB_RS) { DB_RS->close(); DB_RS = nullptr; }
            if (DB_PSTMT) { DB_PSTMT->close(); DB_PSTMT = nullptr; }
            LeaveCriticalSection(&CS_DB_HANDLER);

            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_LOGIN_PLAYER;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResLoginPlayer);

            //ResMsgForOthers.MsgHead.MessageID = (int)EMessageID::S2C_RES_LOGIN_PLAYER;
            //ResMsgForOthers.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            //ResMsgForOthers.MsgHead.MessageSize = sizeof(MessageResLoginPlayer);

            //if (bLoginSuccess)
            //{
            //    //로그인이 성공했을 경우
            //    EnterCriticalSection(&CS_NETWORK_HANDLER);
            //    for (auto itr = CLIENT_POOL.begin(); itr != CLIENT_POOL.end(); ++itr)
            //    {
            //        ResMsg.MsgHead.ReceiverSocketID = (int)itr->first;
            //        if (itr->first == ClientSocket)
            //        {
            //            //로그인이 성공한 자신에게는 Full Information을 전송
            //            retval += send(itr->first, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            //        }
            //        else
            //        {
            //            //다른 사람에게는 PlayerID, PlayerName만 전송 (~~~가 로그인하였습니다.를 처리하기 위함)
            //            retval += send(itr->first, (char*)&ResMsgForOthers, ResMsgForOthers.MsgHead.MessageSize, 0);
            //        }
            //    }
            //    LeaveCriticalSection(&CS_NETWORK_HANDLER);
            //}
            //else
            //{
            //    //로그인이 실패했을 경우, PlayerID 등의 정보가 공란('')인 데이터를 전송
            //    ResMsg.MsgHead.ReceiverSocketID = (int)ClientSocket;
            //    retval += send(ClientSocket, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            //}
            ResMsg.MsgHead.ReceiverSocketID = (int)ClientSocket;
            retval += send(ClientSocket, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            break;
        }
        case EMessageID::C2S_REQ_LOGOUT_PLAYER:
        {
            MessageReqLogoutPlayer ReqMsg = { 0, };
            MessageResLogoutPlayer ResMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqLogoutPlayer));
            EnterCriticalSection(&CS_DB_HANDLER);
            try
            {
                //현재 접속중인 플레이어 정보 업데이트!
                sqlQuery = "UPDATE CURLOGINPLAYER SET    \
                            LOGOUT_DTTM     = NOW(),     \
                            LOGIN_STAT      = 2          \
                        WHERE                            \
                            LOGIN_STAT      = 1 AND      \
                            LOGIN_PLAYER_ID = ?          ";
                sqlQuery = string_ReplaceNSpaceTo1Space(sqlQuery);
                DB_PSTMT = DB_CONN->prepareStatement(sqlQuery);
                DB_PSTMT->setString(1, string(ReqMsg.PLAYER_ID));
                updatedRows = DB_PSTMT->executeUpdate();

                //서버에도 해당정보를 저장!
                EnterCriticalSection(&CS_NETWORK_HANDLER);
                //Clear String
                memset(CLIENT_POOL[ClientSocket].PlayerID, 0, sizeof(CLIENT_POOL[ClientSocket].PlayerID));
                //Clear String
                memset(CLIENT_POOL[ClientSocket].PlayerName, 0, sizeof(CLIENT_POOL[ClientSocket].PlayerName));
                CLIENT_POOL[ClientSocket].LoginYN = false;
                LeaveCriticalSection(&CS_NETWORK_HANDLER);
            }
            catch (sql::SQLException ex)
            {
                std::cout << "[ERR] SQL Error On C2S_REQ_LOGOUT_PLAYER. ErrorMsg : " << ex.what() << std::endl;
            }
            if (DB_RS) { DB_RS->close(); DB_RS = nullptr; }
            if (DB_PSTMT) { DB_PSTMT->close(); DB_PSTMT = nullptr; }
            LeaveCriticalSection(&CS_DB_HANDLER);

            //모든 ClientSocket에게 Broadcast 전송
            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_LOGOUT_PLAYER;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResLogoutPlayer);
            memcpy(ResMsg.LOGOUT_PLAYER_ID, ReqMsg.PLAYER_ID, sizeof(ReqMsg.PLAYER_ID));
            ResMsg.PROCESS_FLAG = updatedRows >= 1 ? (int)EProcessFlag::PROCESS_OK : (int)EProcessFlag::PROCESS_FAIL;

            //EnterCriticalSection(&CS_NETWORK_HANDLER);
            //for (auto itr = CLIENT_POOL.begin(); itr != CLIENT_POOL.end(); ++itr)
            //{
            //    ResMsg.MsgHead.ReceiverSocketID = (int)itr->first;
            //    retval += send(itr->first, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            //}
            //LeaveCriticalSection(&CS_NETWORK_HANDLER);
            ResMsg.MsgHead.ReceiverSocketID = (int)ClientSocket;
            retval += send(ClientSocket, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            break;
        }
        case EMessageID::C2S_REQ_INSERT_SESSIONCHATTINGLOG:
        {
            MessageReqSessionChattingLog ReqMsg = { 0, };
            MessageResSessionChattingLog ResMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqSessionChattingLog));

            EnterCriticalSection(&CS_DB_HANDLER);
            try
            {
                //채팅로그 DB저장
                sqlQuery = "INSERT INTO CHATTINGLOG    \
                            (                          \
                                PLAYER_ID,             \
                                CHATCHANNEL_TYPE,      \
                                SESSION_ID,            \
                                CHAT_MSG,              \
                                REGISTER_DTTM          \
                            )VALUES(                   \
                                ?,                     \
                                ?,                     \
                                ?,                     \
                                ?,                     \
                                NOW()                  \
                            );                         ";
                sqlQuery = string_ReplaceNSpaceTo1Space(sqlQuery);
                DB_PSTMT = DB_CONN->prepareStatement(sqlQuery);
                DB_PSTMT->setString(1, ReqMsg.PLAYER_ID);
                DB_PSTMT->setInt(2, ReqMsg.CHATCHANNEL_TYPE);
                DB_PSTMT->setInt(3, ReqMsg.SESSION_ID);
                DB_PSTMT->setString(4, ReqMsg.CHAT_MSG);
                updatedRows = DB_PSTMT->executeUpdate();
            }
            catch (sql::SQLException ex)
            {
                std::cout << "[ERR] SQL Error On C2S_REQ_INSERT_SESSIONCHATTINGLOG. ErrorMsg : " << ex.what() << std::endl;
            }
            if (DB_RS) { DB_RS->close(); DB_RS = nullptr; }
            if (DB_PSTMT) { DB_PSTMT->close(); DB_PSTMT = nullptr; }
            LeaveCriticalSection(&CS_DB_HANDLER);

            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_INSERT_SESSIONCHATTINGLOG;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResSessionChattingLog);
            ResMsg.CHATCHANNEL_TYPE = ReqMsg.CHATCHANNEL_TYPE;
            ResMsg.SESSION_ID = ReqMsg.SESSION_ID;
            memcpy(ResMsg.PLAYER_ID, ReqMsg.PLAYER_ID, sizeof(ResMsg.PLAYER_ID));
            memcpy(ResMsg.PLAYER_NAME, ReqMsg.PLAYER_NAME, sizeof(ResMsg.PLAYER_NAME));
            memcpy(ResMsg.CHAT_MSG, ReqMsg.CHAT_MSG, sizeof(ResMsg.CHAT_MSG));
            //모든 ClientSocket에게 Broadcast 전송
            EnterCriticalSection(&CS_NETWORK_HANDLER);
            for (auto itr = CLIENT_POOL.begin(); itr != CLIENT_POOL.end(); ++itr)
            {
                ResMsg.MsgHead.ReceiverSocketID = (int)itr->first;
                retval += send(itr->first, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            }
            LeaveCriticalSection(&CS_NETWORK_HANDLER);
            break;
        }
        case EMessageID::C2S_REQ_CREATE_SESSION:
        {
            MessageReqCreateSession ReqMsg = { 0, };
            MessageResCreateSession ResMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqCreateSession));

            EnterCriticalSection(&CS_DB_HANDLER);
            try
            {
                //세션 생성
                sqlQuery = "INSERT INTO GAMESESSION            \
                            (                                  \
                                HOST_PLAYER_ID,                \
                                SESSION_NAME,                  \
                                SESSION_PWD,                   \
                                SESSION_PLAYER,                \
                                SESSION_STATE,                 \
                                SESSION_OPEN_DTTM,             \
                                SESSION_CLOSE_DTTM             \
                            )VALUES(                           \
                                ?,                             \
                                ?,                             \
                                ?,                             \
                                ?,                             \
                                1,                             \
                                NOW(),                         \
                                NULL                           \
                            );                                 ";
                sqlQuery = string_ReplaceNSpaceTo1Space(sqlQuery);
                DB_PSTMT = DB_CONN->prepareStatement(sqlQuery);
                DB_PSTMT->setString(1, string(ReqMsg.HOST_PLAYER_ID));
                DB_PSTMT->setString(2, string(ReqMsg.SESSION_NAME));
                DB_PSTMT->setString(3, string(ReqMsg.SESSION_PASSWORD));
                DB_PSTMT->setInt(4, ReqMsg.SESSION_PLAYER);
                updatedRows += DB_PSTMT->executeUpdate();

                sqlQuery = "SELECT SESSION_ID,SESSION_STATE FROM GAMESESSION WHERE HOST_PLAYER_ID = ? ORDER BY SESSION_OPEN_DTTM DESC LIMIT 1";
                DB_PSTMT = DB_CONN->prepareStatement(sqlQuery);
                DB_PSTMT->setString(1, string(ReqMsg.HOST_PLAYER_ID));
                DB_RS = DB_PSTMT->executeQuery();
                if (DB_RS != nullptr && DB_RS->next() == true)
                {
                    ResMsg.SESSION_ID = DB_RS->getInt("SESSION_ID");
                    ResMsg.SESSION_STATE = DB_RS->getInt("SESSION_STATE");
                }
            }
            catch (sql::SQLException ex)
            {
                std::cout << "[ERR] SQL Error On C2S_REQ_CREATE_SESSION. ErrorMsg : " << ex.what() << std::endl;
            }
            if (DB_RS) { DB_RS->close(); DB_RS = nullptr; }
            if (DB_PSTMT) { DB_PSTMT->close(); DB_PSTMT = nullptr; }
            LeaveCriticalSection(&CS_DB_HANDLER);

            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_CREATE_SESSION;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResCreateSession);
            memcpy(ResMsg.HOST_PLAYER_ID, ReqMsg.HOST_PLAYER_ID, sizeof(ReqMsg.HOST_PLAYER_ID));
            memcpy(ResMsg.HOST_PLAYER_NAME, ReqMsg.HOST_PLAYER_NAME, sizeof(ReqMsg.HOST_PLAYER_NAME));
            memcpy(ResMsg.SESSION_NAME, ReqMsg.SESSION_NAME, sizeof(ReqMsg.SESSION_NAME));
            memcpy(ResMsg.SESSION_PASSWORD, ReqMsg.SESSION_PASSWORD, sizeof(ReqMsg.SESSION_PASSWORD));
            memcpy(ResMsg.SESSION_MAPNAME, ReqMsg.SESSION_MAPNAME, sizeof(ReqMsg.SESSION_MAPNAME));
            ResMsg.SESSION_PLAYER = ReqMsg.SESSION_PLAYER;

            //EnterCriticalSection(&CS_NETWORK_HANDLER);
            //for (auto itr = CLIENT_POOL.begin(); itr != CLIENT_POOL.end(); ++itr)
            //{
            //    ResMsg.MsgHead.ReceiverSocketID = (int)itr->first;
            //    retval += send(itr->first, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            //}
            //LeaveCriticalSection(&CS_NETWORK_HANDLER);

            ResMsg.MsgHead.ReceiverSocketID = (int)ClientSocket;
            retval += send(ClientSocket, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            break;
        }
        case EMessageID::C2S_REQ_UPDATE_PLAYERSTATE:
        {
            MessageReqUpdatePlayerState ReqMsg = { 0, };
            MessageResUpdatePlayerState ResMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqUpdatePlayerState));

            EnterCriticalSection(&CS_DB_HANDLER);
            try
            {
                sqlQuery = "UPDATE PLAYERSTATE SET              \
                                PLAYERCHAR_TYPE         = ?,    \
                                PLAYERCHAR_BODY_SLOT    = ?,    \
                                PLAYERCHAR_HEAD_SLOT    = ?,    \
                                PLAYERCHAR_JUMP_STAT    = ?,    \
                                PLAYERCHAR_STAMINA_STAT = ?,    \
                                PLAYERCHAR_SPEED_STAT   = ?,    \
                                REGISTER_DTTM           = NOW() \
                            WHERE PLAYER_ID = ?                 ";
                sqlQuery = string_ReplaceNSpaceTo1Space(sqlQuery);
                DB_PSTMT = DB_CONN->prepareStatement(sqlQuery);
                DB_PSTMT->setInt(1, ReqMsg.PLAYERCHAR_TYPE);
                DB_PSTMT->setInt(2, ReqMsg.PLAYERCHAR_BODY_SLOT);
                DB_PSTMT->setInt(3, ReqMsg.PLAYERCHAR_HEAD_SLOT);
                DB_PSTMT->setInt(4, ReqMsg.PLAYERCHAR_JUMP_STAT);
                DB_PSTMT->setInt(5, ReqMsg.PLAYERCHAR_STAMINA_STAT);
                DB_PSTMT->setInt(6, ReqMsg.PLAYERCHAR_SPEED_STAT);
                DB_PSTMT->setString(7, ReqMsg.PLAYER_ID);
                updatedRows += DB_PSTMT->executeUpdate();
            }
            catch (sql::SQLException ex)
            {
                std::cout << "[ERR] SQL Error On C2S_REQ_UPDATE_PLAYERSTATE. ErrorMsg : " << ex.what() << std::endl;
            }
            if (DB_RS) { DB_RS->close(); DB_RS = nullptr; }
            if (DB_PSTMT) { DB_PSTMT->close(); DB_PSTMT = nullptr; }
            LeaveCriticalSection(&CS_DB_HANDLER);

            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_UPDATE_PLAYERSTATE;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.ReceiverSocketID = (int)ClientSocket;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResUpdatePlayerState);
            ResMsg.PROCESS_FLAG = updatedRows >= 1 ? (int)EProcessFlag::PROCESS_OK : (int)EProcessFlag::PROCESS_FAIL;
            retval += send(ClientSocket, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            break;
        }
        case EMessageID::C2S_REQ_UPDATE_PLAYERSTATE_REWARD:
        {
            //SQL처리가 실패했을 경우 기존 데이터가 날아가지 않도록 똑같이 Memcpy를 한다.
            MessageReqUpdatePlayerStateReward ReqMsg = { 0, };
            MessageResUpdatePlayerStateReward ResMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqUpdatePlayerStateReward));
            memcpy(&ResMsg, Packet, sizeof(MessageResUpdatePlayerStateReward));

            EnterCriticalSection(&CS_DB_HANDLER);
            try
            {
                //획득골드와 경험치를 업데이트처리한다.
                sqlQuery = "UPDATE PLAYERSTATE SET              \
                                PLAYER_GOLD = PLAYER_GOLD + ?,  \
                                PLAYER_EXP = PLAYER_EXP + ?,    \
                                PLAYER_LEVEL = PLAYER_LEVEL + ?,\
                                REGISTER_DTTM = NOW()           \
                            WHERE PLAYER_ID = ?                 ";
                sqlQuery = string_ReplaceNSpaceTo1Space(sqlQuery);
                DB_PSTMT = DB_CONN->prepareStatement(sqlQuery);
                DB_PSTMT->setInt(1, ReqMsg.EARN_PLAYER_GOLD);
                DB_PSTMT->setInt(2, ReqMsg.EARN_PLAYER_EXP);
                DB_PSTMT->setInt(3, ReqMsg.EARN_PLAYER_LEVEL);
                DB_PSTMT->setString(4, ReqMsg.PLAYER_ID);
                updatedRows += DB_PSTMT->executeUpdate();
                //업데이트된 골드와 경험치를 가져온다.
                sqlQuery = "SELECT PLAYER_GOLD, PLAYER_EXP, PLAYER_LEVEL FROM PLAYERSTATE WHERE PLAYER_ID = ? LIMIT 1";
                DB_PSTMT = DB_CONN->prepareStatement(sqlQuery);
                DB_PSTMT->setString(1, ReqMsg.PLAYER_ID);
                DB_RS = DB_PSTMT->executeQuery();
                if (DB_RS != nullptr && DB_RS->next() == true)
                {
                    ResMsg.UPDATED_PLAYER_GOLD = DB_RS->getInt("PLAYER_GOLD");
                    ResMsg.UPDATED_PLAYER_EXP = DB_RS->getInt("PLAYER_EXP");
                    ResMsg.UPDATED_PLAYER_LEVEL = DB_RS->getInt("PLAYER_LEVEL");
                }
            }
            catch (sql::SQLException ex)
            {
                std::cout << "[ERR] SQL Error On C2S_REQ_UPDATE_PLAYERSTATE_REWARD. ErrorMsg : " << ex.what() << std::endl;
            }
            if (DB_RS) { DB_RS->close(); DB_RS = nullptr; }
            if (DB_PSTMT) { DB_PSTMT->close(); DB_PSTMT = nullptr; }
            LeaveCriticalSection(&CS_DB_HANDLER);

            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_UPDATE_PLAYERSTATE_REWARD;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.ReceiverSocketID = (int)ClientSocket;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResUpdatePlayerStateReward);
            retval += send(ClientSocket, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            break;
        }
        case EMessageID::C2S_REQ_SELECT_PLAYERSTATE:
        {
            MessageReqSelectPlayerState ReqMsg = { 0, };
            MessageResSelectPlayerState ResMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqSelectPlayerState));
            bool bLoginSuccess = false;

            EnterCriticalSection(&CS_DB_HANDLER);
            try
            {
                //플레이어 계정정보와 스탯정보 조회
                sqlQuery = "SELECT                               \
                                PLA.PLAYER_ID,                   \
                                PLA.PLAYER_NAME,                 \
                                PLS.PLAYER_GOLD,                 \
                                PLS.PLAYER_EXP,                  \
                                PLS.PLAYER_LEVEL,                \
                                PLS.PLAYERCHAR_TYPE,             \
                                PLS.PLAYERCHAR_BODY_SLOT,        \
                                PLS.PLAYERCHAR_HEAD_SLOT,        \
                                PLS.PLAYERCHAR_JUMP_STAT,        \
                                PLS.PLAYERCHAR_STAMINA_STAT,     \
                                PLS.PLAYERCHAR_SPEED_STAT        \
                            FROM PLAYER AS PLA                   \
                                INNER JOIN PLAYERSTATE AS PLS    \
                                ON PLA.PLAYER_ID = PLS.PLAYER_ID \
                            WHERE                                \
                                PLA.PLAYER_ID  = ?               \
                            LIMIT 1";
                sqlQuery = string_ReplaceNSpaceTo1Space(sqlQuery);
                DB_PSTMT = DB_CONN->prepareStatement(sqlQuery);
                DB_PSTMT->setString(1, ReqMsg.PLAYER_ID);
                DB_RS = DB_PSTMT->executeQuery();

                if (DB_RS != nullptr && DB_RS->next() == true)
                {
                    //해당 ID가 존재하고, 스탯정보도 존재할 경우 
                    string rsPlayerID = DB_RS->getString("PLAYER_ID").asStdString();
                    memcpy(ResMsg.PLAYER_ID, rsPlayerID.c_str(), rsPlayerID.length());
                    string rsPlayerName = DB_RS->getString("PLAYER_NAME").asStdString();
                    memcpy(ResMsg.PLAYER_NAME, rsPlayerName.c_str(), rsPlayerName.length());

                    ResMsg.PLAYER_GOLD = DB_RS->getInt("PLAYER_GOLD");
                    ResMsg.PLAYER_EXP = DB_RS->getInt("PLAYER_EXP");
                    ResMsg.PLAYER_LEVEL = DB_RS->getInt("PLAYER_LEVEL");
                    ResMsg.PLAYERCHAR_TYPE = DB_RS->getInt("PLAYERCHAR_TYPE");
                    ResMsg.PLAYERCHAR_BODY_SLOT = DB_RS->getInt("PLAYERCHAR_BODY_SLOT");
                    ResMsg.PLAYERCHAR_HEAD_SLOT = DB_RS->getInt("PLAYERCHAR_HEAD_SLOT");
                    ResMsg.PLAYERCHAR_JUMP_STAT = DB_RS->getInt("PLAYERCHAR_JUMP_STAT");
                    ResMsg.PLAYERCHAR_STAMINA_STAT = DB_RS->getInt("PLAYERCHAR_STAMINA_STAT");
                    ResMsg.PLAYERCHAR_SPEED_STAT = DB_RS->getInt("PLAYERCHAR_SPEED_STAT");
                }
            }
            catch (sql::SQLException ex)
            {
                std::cout << "[ERR] SQL Error On C2S_REQ_SELECT_PLAYERSTATE. ErrorMsg : " << ex.what() << std::endl;
            }
            if (DB_RS) { DB_RS->close(); DB_RS = nullptr; }
            if (DB_PSTMT) { DB_PSTMT->close(); DB_PSTMT = nullptr; }
            LeaveCriticalSection(&CS_DB_HANDLER);

            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_SELECT_PLAYERSTATE;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResSelectPlayerState);
            ResMsg.MsgHead.ReceiverSocketID = (int)ClientSocket;
            retval += send(ClientSocket, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            break;
        }
        default:
        {
            std::cout << "[ERR] Invalid Message Format! " << std::endl;
            retval = 1;
            break;
        }
    }
    return retval;
}

unsigned WINAPI ThreadProcessClientSocket(void* arg)
{
    SOCKET ClientSocket = *(SOCKET*)arg;
    HANDLE ThreadHandle = THREAD_POOL[ClientSocket];
    char RecvBuffer[NET_PACKET_SIZE] = { 0, };
    char SendBuffer[NET_PACKET_SIZE] = { 0, };
    int RecvBytes = 0;
    int SendBytes = 0;
    int Retval = 0;

    while (G_PROGRAMRUNNING)
    {
        memset(RecvBuffer, 0, sizeof(RecvBuffer));
        RecvBytes = recv(ClientSocket, RecvBuffer, sizeof(RecvBuffer), 0);
        if (RecvBytes <= 0) { break; }
        Retval = ProcessPacket(ClientSocket, &RecvBuffer[0]);
        //if (Retval <= 0) { break; }
    }
    if (G_PROGRAMRUNNING)
    {
        /*--------------------------------------------------------------------------
        TCP Socket recv에서 SocketClient Connection이 끊어지거나 에러가 발생할 경우 RecvBytes = 0 또는 -1을 리턴한다.
        ProcessPacket에서는 각 SocketClient로 Unicast/Broadcast Send를 하나, 여기서는 별도로 연결끊김 체크를 하지 않는다.(주석처리)
        --------------------------------------------------------------------------*/
        MessageHeader msgHead = { 0, };
        msgHead.MessageID = (int)EMessageID::S2C_RES_CLINET_DISCONNET;
        msgHead.MessageSize = sizeof(MessageHeader);
        msgHead.SenderSocketID = (int)NET_SERVERSOCKET;
        EnterCriticalSection(&CS_NETWORK_HANDLER);
        for (auto itr = CLIENT_POOL.begin(); itr != CLIENT_POOL.end(); ++itr)
        {
            if (itr->first == ClientSocket)
            {
                CLIENT_POOL.erase(itr);
                break;
            }
        }
        closesocket(ClientSocket);
        std::cout << "[SYS] ClilentSocket [" << ClientSocket << "] DisConnected!" << std::endl;
        for (auto itr = CLIENT_POOL.begin(); itr != CLIENT_POOL.end(); ++itr)
        {
            msgHead.ReceiverSocketID = (int)itr->first;
            send(itr->first, (char*)&msgHead, msgHead.MessageSize, 0);
        }
        LeaveCriticalSection(&CS_NETWORK_HANDLER);
    }
    return 0;
}

unsigned WINAPI ThreadProcessDatabase(void* arg)
{
    /*-----------------------------------------------------------------------------------
    ※ DB EXECUTEQUERY ISSUE
    1. 하나의 DB_CONNection에서 SQLPreparedStatement는 16382번 이상 쿼리수행 시
      [Can't create more than max_prepared_stmt_count statements] Exception이 발생한다.

    2. SQL문을 처리할 때 DB_CONNection을 매번 Connect/Close 하면 Unable to connect to localhost가 뜬다.
       그렇다고 너무 많은 DB_CONNection을 Connect할 경우, Connect Count에 걸려서 Connect가 되지 않는다.

    [1] Pstmt 대신 stmt를 쓰면 어떨까?
    [2] 아니면 16382번 돌기 전에 Connection 끊고 다시 Reconnect하면 어떨까?
    [3] stackoverflow 검색결과 한번에 하나 생성하고 돌리고 지우며, stmt 다쓰면 close시키라고 한다.
    [4] 이것도 안되면 set global max_prepared_stmt_count를 증가시키라고 한다...

    ** 일단 대부분의 기능이 동작하도록 구성해두고, 나중에 이 이슈를 해결해야 한다!!
    -----------------------------------------------------------------------------------*/
    int ThreadIdx = *(int*)arg;
    HANDLE ThreadHandle = THREAD_POOL[ThreadIdx];

    while (G_PROGRAMRUNNING)
    {
        string sqlQuery = "INSERT INTO TESTTABLE(TESTCOL_NAME,REGDATE)VALUES(?,NOW())";
        sql::Statement* sqlStmt = nullptr;
        sql::PreparedStatement* DB_PSTMT = nullptr;
        sql::ResultSet* DB_RS = nullptr;
        int sqlResultRows = 0;

        EnterCriticalSection(&CS_DB_HANDLER);
        try
        {
            DB_PSTMT = DB_CONN->prepareStatement(sqlQuery);
            DB_PSTMT->setString(1, string_format("Thread[%d] : HelloWorld!", ThreadIdx));
            sqlResultRows = DB_PSTMT->executeUpdate();
            std::cout << string_format("Thread[%d] : InsertRows : [%d]\n", ThreadIdx, sqlResultRows).c_str() << std::endl;
        }
        catch (sql::SQLException e)
        {
            std::cout << "[ERR] DB_CONNection Error Occurred. ErrorMsg : " << e.what() << std::endl;
        }
        if (DB_PSTMT) { DB_PSTMT->close(); }
        WaitForSingleObject(ThreadHandle, (1000 / 60));
    }
    return 0;
}

#ifdef _MICRO_TEST_ENABLED
int GameServerCPPv4_main(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif 
{
    std::cout << "AnimalGuys Server v20221228" << std::endl;

    InitializeCriticalSection(&CS_THREAD_HANDLER);
    InitializeCriticalSection(&CS_NETWORK_HANDLER);
    InitializeCriticalSection(&CS_DB_HANDLER);

    try
    {
        DB_DRIVER = get_driver_instance();
        DB_CONN = DB_DRIVER->connect(DB_SERVERNAME, DB_USERNAME, DB_PASSWORD);
        DB_CONN->setSchema(DB_DBNAME);
        std::cout << "[SYS] MySQLDB [" << DB_SERVERNAME << "] Connected!" << std::endl;
    }
    catch (sql::SQLException e)
    {
        std::cout << "[ERR] DB_CONNection Error Occurred. ErrorMsg : " << e.what() << std::endl;
        exit(-11);
    }

    if (WSAStartup(MAKEWORD(2, 2), &NET_WSADATA) != 0)
    {
        std::cout << "[ERR] WSAStartup Error Occurred. ErrorCode : " << GetLastError() << std::endl;
        exit(-1);
    }

    NET_SERVERSOCKET = socket(AF_INET, SOCK_STREAM, 0);
    if (NET_SERVERSOCKET == INVALID_SOCKET)
    {
        std::cout << "[ERR] ServerSocket Creation Error Occurred. ErrorCode : " << GetLastError() << std::endl;
        exit(-2);
    }

    NET_SERVERADDR.sin_family = AF_INET; // IPV4
    NET_SERVERADDR.sin_addr.S_un.S_addr = inet_addr(NET_SERVER_IPV4);
    NET_SERVERADDR.sin_port = htons(NET_SERVER_PORT);
    if (::bind(NET_SERVERSOCKET, (SOCKADDR*)&NET_SERVERADDR, sizeof(NET_SERVERADDR)) != 0)
    {
        std::cout << "[ERR] ServerSocket Bind Error Occurred. ErrorCode : " << GetLastError() << std::endl;
        exit(-3);
    };

    if (listen(NET_SERVERSOCKET, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cout << "[ERR] ServerSocket Listen Error Occurred. ErrorCode : " << GetLastError() << std::endl;
        exit(-4);
    };

    std::cout << "[SYS] ServerSocket [" << NET_SERVERSOCKET << "] Listen Started!" << std::endl;
    while (G_PROGRAMRUNNING)
    {
        SOCKADDR_IN ClientAddr = { 0, };
        int szClientAddr = sizeof(ClientAddr);
        SOCKET ClientSocket = accept(NET_SERVERSOCKET, (SOCKADDR*)&ClientAddr, &szClientAddr);
        if (ClientSocket == INVALID_SOCKET)
        {
            closesocket(ClientSocket);
            continue;
        }
        EnterCriticalSection(&CS_NETWORK_HANDLER);
        CLIENT_POOL[ClientSocket] = ClientData(ClientSocket);
        std::cout << "[SYS] ClientSocket [" << ClientSocket << "] Connected!" << std::endl;
        MessageHeader msgHead = { 0, };
        msgHead.MessageID = (int)EMessageID::S2C_RES_CLINET_CONNECT;
        msgHead.MessageSize = sizeof(MessageHeader);
        msgHead.SenderSocketID = (int)NET_SERVERSOCKET;
        msgHead.ReceiverSocketID = (int)ClientSocket;
        send(ClientSocket, (char*)&msgHead, msgHead.MessageSize, 0);
        LeaveCriticalSection(&CS_NETWORK_HANDLER);
        EnterCriticalSection(&CS_THREAD_HANDLER);
        THREAD_POOL[ClientSocket] = ((HANDLE)_beginthreadex(nullptr, 0, ThreadProcessClientSocket, (void*)&ClientSocket, 0, nullptr));
        LeaveCriticalSection(&CS_THREAD_HANDLER);
    }
    std::cout << "[SYS] ServerSocket [" << NET_SERVERSOCKET << "] Listen Finished!" << std::endl;
    closesocket(NET_SERVERSOCKET);

    EnterCriticalSection(&CS_THREAD_HANDLER);
    for (auto itr = THREAD_POOL.begin(); itr != THREAD_POOL.end(); ++itr)
    {
        WaitForSingleObject(itr->second, INFINITE);
        CloseHandle(itr->second);
    }
    LeaveCriticalSection(&CS_THREAD_HANDLER);
    DeleteCriticalSection(&CS_THREAD_HANDLER);
    DeleteCriticalSection(&CS_NETWORK_HANDLER);
    DeleteCriticalSection(&CS_DB_HANDLER);
    
    if (DB_RS) { DB_RS->close(); DB_RS = nullptr; }
    if (DB_PSTMT) { DB_PSTMT->close(); DB_PSTMT = nullptr; }
    if (DB_CONN) { DB_CONN->close(); DB_CONN = nullptr; }

    WSACleanup();
    return 0;
}

#ifdef _MICRO_TEST_ENABLED
int main(int argc, char* argv[])
#else
int MicroTest_main(int argc, char* argv[])
#endif 
{
    InitializeCriticalSection(&CS_DB_HANDLER);
    DB_DRIVER = get_driver_instance();
    DB_CONN = DB_DRIVER->connect(DB_SERVERNAME, DB_USERNAME, DB_PASSWORD);
    DB_CONN->setSchema(DB_DBNAME);
    string strCommand = "";
    int iList[100] = { 0, };
    for (int i = 0; i < 100; i++)
    {
        /*------------------------------------------------------------------------
        _beginthreadex의 파라미터는 Stack메모리에 저장되는 자료형으로 보내는게 좋을 것 같다.
        왜인지 모르겠지만, 파라미터가 Heap메모리에 저장되는 자료형을 쓸 경우, ThreadRun이 빨리 돌아서 자료형이 생성되기도 전에
        참조를 걸어서 NullPtr를 참조하는 현상이 발생되었기 때문이다.
        100명의 Client가 접속해서 1프레임마다 DBCall을 하는 로직을 실행한 결과, 여러 문제점들이 발생한다.
        일단은 구현을 끝내고 유지보수 단계에서 해당 문제를 해결하자!
        ------------------------------------------------------------------------*/
        iList[i] = i;
        THREAD_POOL[i] = ((HANDLE)_beginthreadex(nullptr, 0, ThreadProcessDatabase, (void*)&iList[i], 0, nullptr));
    }
    std::cout << "[SYS] All Threads(100) Started!" << std::endl;
    while (G_PROGRAMRUNNING)
    {
        std::getline(std::cin, strCommand);
        if (strCommand == "q")
        {
            G_PROGRAMRUNNING = false;
            break;
        }
    }
    for (auto itr = THREAD_POOL.begin(); itr != THREAD_POOL.end(); ++itr)
    {
        WaitForSingleObject(itr->second, INFINITE);
        CloseHandle(itr->second);
    }
    DeleteCriticalSection(&CS_DB_HANDLER);
    return 0;
}