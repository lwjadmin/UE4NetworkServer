#include "GameServerCPPv4.h"


using namespace std;

int ProcessPacket(SOCKET ClientSocket, char* buffer)
{
    int retval = 0;
    char Packet[NET_PACKET_SIZE] = { 0, };
    string strQuery = "";
    memcpy(Packet, buffer, sizeof(Packet));
    MessageHeader MsgHead = { 0, };
    memcpy(&MsgHead, Packet, sizeof(MsgHead));

    switch ((EMessageID)MsgHead.MessageID)
    {
        case EMessageID::C2S_REQ_INSERT_PLAYER:
        {
            /*------------------------------------------------------
            회원ID가 DB에 있는지 확인하고, 없으면 INSERT하고 Null반환, 
            있으면 회원ID 반환
            ------------------------------------------------------*/
            MessageReqInsertPlayer ReqMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqInsertPlayer));
            strQuery = "SELECT PLAYER_ID FROM PLAYER WHERE PLAYER_ID = ? LIMIT 1";
            DB_PSTMT = DB_CONN->prepareStatement(strQuery);
            DB_PSTMT->setString(1, ReqMsg.PLAYER_ID);
            DB_RS = DB_PSTMT->executeQuery();

            string OUT_PLAYER_ID = "";
            while (DB_RS->next())
            {
                OUT_PLAYER_ID = DB_RS->getString("PLAYER_ID").asStdString();
            }

            int UpdateRows = 0;
            if (OUT_PLAYER_ID == "")
            {
                strQuery = "INSERT INTO PLAYER(PLAYER_ID,PLAYER_PWD,PLAYER_NAME)VALUES(?,?,?)";
                DB_PSTMT = DB_CONN->prepareStatement(strQuery);
                DB_PSTMT->setString(1, ReqMsg.PLAYER_ID);
                DB_PSTMT->setString(2, ReqMsg.PLAYER_NAME);
                DB_PSTMT->setString(3, ReqMsg.PLAYER_PWD);
                UpdateRows = DB_PSTMT->executeUpdate() >= 1 ? (int)EProcessFlag::PROCESS_OK : (int)EProcessFlag::PROCESS_FAIL;
            }

            MessageResInsertPlayer ResMsg = { 0, };
            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_INSERT_PLAYER;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.ReceiverSocketID = (int)ClientSocket;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResInsertPlayer);
            ResMsg.PROCESS_FLAG = UpdateRows;
            retval = send(ClientSocket, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            break;
        }
        case EMessageID::C2S_REQ_LOGIN_PLAYER:
        {
            MessageReqLoginPlayer ReqMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqLoginPlayer));
            strQuery = "SELECT                               \
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
                            PLA.PLAYER_PWD = ?               ";
            strQuery = string_ReplaceNSpaceTo1Space(strQuery);
            DB_PSTMT = DB_CONN->prepareStatement(strQuery);
            DB_PSTMT->setString(1, string(ReqMsg.PLAYER_ID));
            DB_PSTMT->setString(2, string(ReqMsg.PLAYER_PWD));
            DB_RS = DB_PSTMT->executeQuery();

            MessageResLoginPlayer ResMsg = { 0, };
            bool isExists = false;
            while (DB_RS->next())
            {
                string rsPlayerID = DB_RS->getString("PLAYER_ID").asStdString();
                memcpy(ResMsg.PLAYER_ID, rsPlayerID.c_str(), rsPlayerID.length());
                string rsPlayerPWD = DB_RS->getString("PLAYER_PWD").asStdString();
                memcpy(ResMsg.PLAYER_PWD, rsPlayerPWD.c_str(), rsPlayerPWD.length());
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
                isExists = true;
            }
            if (isExists)
            {
                strQuery = "INSERT INTO CURLOGINPLAYER(LOGIN_PLAYER_ID,LOGIN_DTTM,LOGOUT_DTTM,LOGIN_STAT)VALUES(?,NOW(),NULL,1)";
                DB_PSTMT = DB_CONN->prepareStatement(strQuery);
                DB_PSTMT->setString(1, string(ReqMsg.PLAYER_ID));
                DB_PSTMT->execute();
            }
            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_LOGIN_PLAYER;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.ReceiverSocketID = (int)ClientSocket;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResLoginPlayer);
            retval = send(ClientSocket, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            break;
        }
        case EMessageID::C2S_REQ_LOGOUT_PLAYER:
        {
            MessageReqLogoutPlayer ReqMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqLogoutPlayer));
            strQuery = "UPDATE CURLOGINPLAYER SET    \
                            LOGOUT_DTTM     = NOW(), \
                            LOGIN_STAT      = 2      \
                        WHERE                        \
                            LOGIN_STAT      = 1 AND  \
                            LOGIN_PLAYER_ID = ?      ";
            strQuery = string_ReplaceNSpaceTo1Space(strQuery);
            DB_PSTMT = DB_CONN->prepareStatement(strQuery);
            DB_PSTMT->setString(1, string(ReqMsg.PLAYER_ID));
            int UpdateRows = DB_PSTMT->executeUpdate();
            MessageResLogoutPlayer ResMsg = { 0, };
            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_LOGOUT_PLAYER;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.ReceiverSocketID = (int)ClientSocket;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResLogoutPlayer);
            ResMsg.PROCESS_FLAG = UpdateRows >= 1 ? (int)EProcessFlag::PROCESS_OK : (int)EProcessFlag::PROCESS_FAIL;
            retval = send(ClientSocket, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            break;
        }
        case EMessageID::C2S_REQ_INSERT_SESSIONCHATTINGLOG:
        {
            //요청메시지 해석-------------------------------------------------------------
            MessageReqSessionChattingLog ReqMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqSessionChattingLog));
            //쿼리 처리-------------------------------------------------------------------
            strQuery = "INSERT INTO CHATTINGLOG    \
                        (                          \
                            PLAYER_ID,             \
                            CHATCHANNEL_TYPE,      \
                            SESSION_ID,            \
                            CHAT_MSG,              \
                            REGISTER_DTTM          \
                        )VALUES(                   \
                            ? ,                    \
                            1,                     \
                            NULL,                  \
                            ? ,                    \
                            NOW()                  \
                        );                         ";
            strQuery = string_ReplaceNSpaceTo1Space(strQuery);
            DB_PSTMT = DB_CONN->prepareStatement(strQuery);
            DB_PSTMT->setString(1, string(ReqMsg.PLAYER_ID));
            DB_PSTMT->setString(2, string(ReqMsg.CHAT_MSG));
            DB_PSTMT->execute();
            //요청메시지 응답(BROADCAST)----------------------------------------------------
            MessageResSessionChattingLog ResMsg = { 0, };
            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_INSERT_SESSIONCHATTINGLOG;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResSessionChattingLog);
            memcpy(ResMsg.PLAYER_ID, ReqMsg.PLAYER_ID, sizeof(ResMsg.PLAYER_ID));
            memcpy(ResMsg.PLAYER_NAME, ReqMsg.PLAYER_NAME, sizeof(ResMsg.PLAYER_NAME));
            memcpy(ResMsg.CHAT_MSG, ReqMsg.CHAT_MSG, sizeof(ResMsg.CHAT_MSG));
            EnterCriticalSection(&CS_NETWORK_HANDLER);
            for (auto itr = CLIENT_POOL.begin(); itr != CLIENT_POOL.end(); ++itr)
            {
                ResMsg.MsgHead.ReceiverSocketID = (int)itr->first;
                send(itr->first, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            }
            LeaveCriticalSection(&CS_NETWORK_HANDLER);
            retval = 1;
            break;
        }
        case EMessageID::C2S_REQ_CREATE_SESSION:
        {
            //요청메시지 해석-------------------------------------------------------------
            MessageReqCreateSession ReqMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqCreateSession));
            //쿼리 처리-------------------------------------------------------------------
            strQuery = "INSERT INTO GAMESESSION            \
                        (                                  \
                            HOST_PLAYER_ID,                \
                            SESSION_NAME,                  \
                            SESSION_PWD,                   \
                            SESSION_PLAYER,                \
                            SESSION_STATE,                 \
                            SESSION_OPEN_DTTM,             \
                            SESSION_CLOSE_DTTM             \
                        )                                  \
                        VALUES                             \
                        (                                  \
                            ?,                             \
                            ?,                             \
                            ?,                             \
                            ?,                             \
                            1,                             \
                            NOW(),                         \
                            NULL                           \
                        );                                 ";
            strQuery = string_ReplaceNSpaceTo1Space(strQuery);
            DB_PSTMT = DB_CONN->prepareStatement(strQuery);
            DB_PSTMT->setString(1, string(ReqMsg.HOST_PLAYER_ID));
            DB_PSTMT->setString(2, string(ReqMsg.SESSION_NAME));
            DB_PSTMT->setString(3, string(ReqMsg.SESSION_PASSWORD));
            DB_PSTMT->setInt(4, ReqMsg.SESSION_PLAYER);
            DB_PSTMT->execute();

            strQuery = "SELECT * FROM GAMESESSION WHERE HOST_PLAYER_ID = ? ORDER BY SESSION_OPEN_DTTM ASC LIMIT 1";
            strQuery = string_ReplaceNSpaceTo1Space(strQuery);
            DB_PSTMT = DB_CONN->prepareStatement(strQuery);
            DB_PSTMT->setString(1, string(ReqMsg.HOST_PLAYER_ID));
            DB_RS = DB_PSTMT->executeQuery();

            //요청메시지 응답(BROADCAST)----------------------------------------------------
            MessageResCreateSession ResMsg = { 0, };
            while (DB_RS->next())
            {
                string rsPlayerName = DB_RS->getString("PLAYER_NAME").asStdString();
                int rsSessionState = DB_RS->getInt("SESSION_STATE");
                int rsSessionID = DB_RS->getInt("SESSION_ID");

                ResMsg.SESSION_ID = rsSessionID;
                ResMsg.SESSION_STATE = rsSessionState;
                memcpy(ResMsg.HOST_PLAYER_NAME, rsPlayerName.c_str(), rsPlayerName.length());
            }
            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_CREATE_SESSION;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResCreateSession);
            memcpy(ResMsg.HOST_PLAYER_ID, ReqMsg.HOST_PLAYER_ID, sizeof(ReqMsg.HOST_PLAYER_ID));
            EnterCriticalSection(&CS_NETWORK_HANDLER);
            for (auto itr = CLIENT_POOL.begin(); itr != CLIENT_POOL.end(); ++itr)
            {
                ResMsg.MsgHead.ReceiverSocketID = (int)itr->first;
                send(itr->first, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            }
            LeaveCriticalSection(&CS_NETWORK_HANDLER);
            break;
        }
        case EMessageID::C2S_REQ_UPDATE_PLAYERSTATE:
        {
            //요청메시지 해석-------------------------------------------------------------
            MessageReqUpdatePlayerState ReqMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqUpdatePlayerState));
            //쿼리 처리-------------------------------------------------------------------
            strQuery = "UPDATE PLAYERSTATE SET              \
                            PLAYERCHAR_TYPE         = ?,    \
                            PLAYERCHAR_BODY_SLOT    = ?,    \
                            PLAYERCHAR_HEAD_SLOT    = ?,    \
                            PLAYERCHAR_JUMP_STAT    = ?,    \
                            PLAYERCHAR_STAMINA_STAT = ?,    \
                            PLAYERCHAR_SPEED_STAT   = ?,    \
                            REGISTER_DTTM           = NOW() \
                        WHERE PLAYER_ID = ?                 ";
            strQuery = string_ReplaceNSpaceTo1Space(strQuery);
            DB_PSTMT = DB_CONN->prepareStatement(strQuery);
            DB_PSTMT->setInt(1, ReqMsg.PLAYERCHAR_TYPE);
            DB_PSTMT->setInt(2, ReqMsg.PLAYERCHAR_BODY_SLOT);
            DB_PSTMT->setInt(3, ReqMsg.PLAYERCHAR_HEAD_SLOT);
            DB_PSTMT->setInt(4, ReqMsg.PLAYERCHAR_JUMP_STAT);
            DB_PSTMT->setInt(5, ReqMsg.PLAYERCHAR_STAMINA_STAT);
            DB_PSTMT->setInt(6, ReqMsg.PLAYERCHAR_SPEED_STAT);
            DB_PSTMT->setString(7, ReqMsg.PLAYER_ID);
            int UpdateRows = DB_PSTMT->executeUpdate();
            //요청메시지 응답(UNICAST)------------------------------------------------------
            MessageResUpdatePlayerState ResMsg = { 0, };
            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_UPDATE_PLAYERSTATE;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.ReceiverSocketID = (int)ClientSocket;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResUpdatePlayerState);
            ResMsg.PROCESS_FLAG = UpdateRows >= 1 ? (int)EProcessFlag::PROCESS_OK : (int)EProcessFlag::PROCESS_FAIL;
            retval = send(ClientSocket, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            break;
        }
        default :
        {
            cout << "[ERR] Invalid Message Format! " << endl;
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
        if (Retval <= 0) { break; }
    }
    if (G_PROGRAMRUNNING)
    {
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
        cout << "[SYS] ClilentSocket [" << ClientSocket << "] DisConnected!" << endl;
        for (auto itr = CLIENT_POOL.begin(); itr !=  CLIENT_POOL.end(); ++itr)
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
    1. 하나의 SQLConnection에서 SQLPreparedStatement는 16382번 이상 쿼리수행 시
      [Can't create more than max_prepared_stmt_count statements] Exception이 발생한다.

    2. SQL문을 처리할 때 SQLConnection을 매번 Connect/Close 하면 Unable to connect to localhost가 뜬다.
       그렇다고 너무 많은 SQLConnection을 Connect할 경우, Connect Count에 걸려서 Connect가 되지 않는다.

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
        sql::PreparedStatement* sqlPstmt = nullptr;
        sql::ResultSet* sqlRs = nullptr;
        int sqlResultRows = 0;

        EnterCriticalSection(&CS_DB_HANDLER);
        try
        {
            sqlPstmt = DB_CONN->prepareStatement(sqlQuery);
            sqlPstmt->setString(1, string_format("Thread[%d] : HelloWorld!", ThreadIdx));
            sqlResultRows = sqlPstmt->executeUpdate();
            cout << string_format("Thread[%d] : InsertRows : [%d]\n", ThreadIdx, sqlResultRows).c_str() << endl;
        }
        catch (sql::SQLException e)
        {
            cout << "[ERR] SQLConnection Error Occurred. ErrorMsg : " << e.what() << endl;
        }
        if (sqlPstmt) { sqlPstmt->close(); }
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
    InitializeCriticalSection(&CS_NETWORK_HANDLER);
    InitializeCriticalSection(&CS_THREAD_HANDLER);
    InitializeCriticalSection(&CS_THREAD_HANDLER);

    try
    {
        DB_DRIVER = get_driver_instance();
        DB_CONN = DB_DRIVER->connect(DB_SERVERNAME, DB_USERNAME, DB_PASSWORD);
        DB_CONN->setSchema(DB_DBNAME);
    }
    catch (sql::SQLException e)
    {
        cout << "[ERR] SQLConnection Error Occurred. ErrorMsg : " << e.what() << endl;
        exit(-100);
    }

    if (WSAStartup(MAKEWORD(2, 2), &NET_WSADATA) != 0)
    {
        cout << "[ERR] WSAStartup Error Occurred. ErrorCode : " << GetLastError() << endl;
        exit(-1);
    }
    
    NET_SERVERSOCKET = socket(AF_INET, SOCK_STREAM, 0);
    if (NET_SERVERSOCKET == INVALID_SOCKET)
    {
        cout << "[ERR] ServerSocket Creation Error Occurred. ErrorCode : " << GetLastError() << endl;
        exit(-2);
    }
    
    NET_SERVERADDR.sin_family = AF_INET; // IPV4
    NET_SERVERADDR.sin_addr.S_un.S_addr = inet_addr(NET_SERVER_IPV4);
    NET_SERVERADDR.sin_port = htons(NET_SERVER_PORT);
    if (::bind(NET_SERVERSOCKET, (SOCKADDR*)&NET_SERVERADDR, sizeof(NET_SERVERADDR)) != 0)
    {
        cout << "[ERR] ServerSocket Bind Error Occurred. ErrorCode : " << GetLastError() << endl;
        exit(-3);
    };
    
    if (listen(NET_SERVERSOCKET, SOMAXCONN) == SOCKET_ERROR)
    {
        cout << "[ERR] ServerSocket Listen Error Occurred. ErrorCode : " << GetLastError() << endl;
        exit(-4);
    };
    
    cout << "[SYS] ServerSocket [" << NET_SERVERSOCKET << "] Listen Started!" << endl;
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
        cout << "[SYS] ClientSocket [" << ClientSocket << "] Connected!" << endl;
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
    cout << "[SYS] ServerSocket [" << NET_SERVERSOCKET << "] Listen Finished!" << endl;
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
    
    WSACleanup();
    DB_CONN->close();
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
    int iList[100] = {0,};
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
    cout << "[SYS] All Threads(100) Started!" << endl;
    while (G_PROGRAMRUNNING)
    {
        getline(cin, strCommand);
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