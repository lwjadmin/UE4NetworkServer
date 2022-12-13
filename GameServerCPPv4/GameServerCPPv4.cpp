#define _WINSOCK_DEPRECATED_NO_WARNINGS
//
#include <WinSock2.h> 
#include <Windows.h>
#include <process.h>
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <string>
//
#include "jdbc/mysql_connection.h"
#include "jdbc/cppconn/driver.h"
#include "jdbc/cppconn/exception.h"
#include "jdbc/cppconn/prepared_statement.h"
//
#include "StringHelper.h"
#include "MessagePacket.h"
//
#pragma comment (lib, "WS2_32.lib")
#pragma comment (lib, "debug/mysqlcppconn.lib")
//
using namespace std;
//
const char* NET_SERVER_IPV4 = "172.16.2.146";
const int NET_SERVER_PORT = 5001;
const int NET_PACKET_SIZE = 512;
//                      
WSAData NET_WSADATA = { 0, };
SOCKET NET_SERVERSOCKET = NULL;
SOCKADDR_IN NET_SERVERADDR = { 0, };
//                      
const string DB_SERVERNAME = "tcp://172.16.2.146:3306";
const string DB_USERNAME = "AnimalGuysAdmin";
const string DB_PASSWORD = "Passw0rd";
const string DB_DBNAME = "ANIMALGUYS";
//                      
vector<SOCKET> NET_CLIENTLIST;
CRITICAL_SECTION CS_NET_CLIENTLIST;
//                      
map<SOCKET, HANDLE> NET_THREADLIST;
CRITICAL_SECTION CS_NET_THREADLIST;
//
sql::Driver* DB_DRIVER = nullptr;
sql::Connection* DB_CONN = nullptr;
sql::Statement* DB_STMT = nullptr;
sql::PreparedStatement* DB_PSTMT = nullptr;
sql::ResultSet* DB_RS = nullptr;
//
bool G_ProgramRunning = true;

int ProcessPacket(SOCKET ClientSocket, char* buffer)
{
    int retval = 0;
    char Packet[NET_PACKET_SIZE] = { 0, };
    string strQuery = "";
    memcpy(buffer, Packet, sizeof(Packet));
    MessageHeader MsgHead = { 0, };
    memcpy(&MsgHead, Packet, sizeof(MsgHead));

    switch ((EMessageID)MsgHead.MessageID)
    {
        case EMessageID::C2S_REQ_INSERT_PLAYER:
        {
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

            if (OUT_PLAYER_ID != "")
            {
                strQuery = "INSERT INTO PLAYER(PLAYER_ID,PLAYER_PWD,PLAYER_NAME)VALUES(?,?,?)";
                DB_PSTMT = DB_CONN->prepareStatement(strQuery);
                DB_PSTMT->setString(1, ReqMsg.PLAYER_ID);
                DB_PSTMT->setString(2, ReqMsg.PLAYER_NAME);
                DB_PSTMT->setString(3, ReqMsg.PLAYER_PWD);
                DB_PSTMT->execute();
            }

            MessageResInsertPlayer ResMsg = { 0, };
            ResMsg.MsgHead.MessageID = (int)EMessageID::S2C_RES_INSERT_PLAYER;
            ResMsg.MsgHead.SenderSocketID = (int)NET_SERVERSOCKET;
            ResMsg.MsgHead.ReceiverSocketID = (int)ClientSocket;
            ResMsg.MsgHead.MessageSize = sizeof(MessageResInsertPlayer);
            memcpy(ResMsg.PLAYER_ID, OUT_PLAYER_ID.c_str(), OUT_PLAYER_ID.length());
            retval = send(ClientSocket, (char*)&ResMsg, ResMsg.MsgHead.MessageSize, 0);
            break;
        }
        case EMessageID::C2S_REQ_LOGIN_PLAYER:
        {
            MessageReqInsertPlayer ReqMsg = { 0, };
            memcpy(&ReqMsg, Packet, sizeof(MessageReqInsertPlayer));
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
    char RecvBuffer[NET_PACKET_SIZE] = { 0, };
    char SendBuffer[NET_PACKET_SIZE] = { 0, };
    int RecvBytes = 0;
    int SendBytes = 0;
    int Retval = 0;

    while (G_ProgramRunning)
    {
        memset(RecvBuffer, 0, sizeof(RecvBuffer));
        RecvBytes = recv(ClientSocket, RecvBuffer, sizeof(RecvBuffer), 0);
        if (RecvBytes <= 0) { break; }
        Retval = ProcessPacket(ClientSocket, &RecvBuffer[0]);
        if (Retval <= 0) { break; }
    }
    if (G_ProgramRunning)
    {
        MessageHeader msgHead = { 0, };
        msgHead.MessageID = (int)EMessageID::S2C_RES_CLINET_DISCONNET;
        msgHead.MessageSize = sizeof(MessageHeader);
        msgHead.SenderSocketID = (int)NET_SERVERSOCKET;
        EnterCriticalSection(&CS_NET_CLIENTLIST);
        NET_CLIENTLIST.erase(find(NET_CLIENTLIST.begin(), NET_CLIENTLIST.end(), ClientSocket));
        closesocket(ClientSocket);
        cout << "[SYS] Socket [" << ClientSocket << "] DisConnected!" << endl;
        for (auto itr = NET_CLIENTLIST.begin(); itr !=  NET_CLIENTLIST.end(); ++itr)
        {
            msgHead.ReceiverSocketID = (int)(*itr);
            send(*itr, (char*)&msgHead, msgHead.MessageSize, 0);
        }
        LeaveCriticalSection(&CS_NET_CLIENTLIST);
    }
    return 0;
}


int main(int argc, char* argv[])
{
    InitializeCriticalSection(&CS_NET_CLIENTLIST);
    InitializeCriticalSection(&CS_NET_THREADLIST);

    try
    {
        DB_DRIVER = get_driver_instance();
        DB_CONN = DB_DRIVER->connect(DB_SERVERNAME, DB_USERNAME, DB_PASSWORD);
        DB_CONN->setSchema(DB_DBNAME);
    }
    catch (sql::SQLException e)
    {
        cout << "[ERR] SQLConnection Error Occurred. ErrorMsg : " << e.what() << endl;
        exit(-11);
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

    cout << "[SYS] Server Started!" << endl;
    while (G_ProgramRunning)
    {
        SOCKADDR_IN ClientAddr = { 0, };
        int szClientAddr = sizeof(ClientAddr);
        SOCKET ClientSocket = accept(NET_SERVERSOCKET, (SOCKADDR*)&ClientAddr, &szClientAddr);
        if (ClientSocket == INVALID_SOCKET)
        {
            closesocket(ClientSocket);
            continue;
        }
        EnterCriticalSection(&CS_NET_CLIENTLIST);
        NET_CLIENTLIST.push_back(ClientSocket);
        cout << "[SYS] New Socket [" << ClientSocket << "] Connected!" << endl;
        MessageHeader msgHead = { 0, };
        msgHead.MessageID = (int)EMessageID::S2C_RES_CLINET_CONNECT;
        msgHead.MessageSize = sizeof(MessageHeader);
        msgHead.SenderSocketID = (int)NET_SERVERSOCKET;
        msgHead.ReceiverSocketID = (int)ClientSocket;
        send(ClientSocket, (char*)&msgHead, msgHead.MessageSize, 0);
        LeaveCriticalSection(&CS_NET_CLIENTLIST);
        EnterCriticalSection(&CS_NET_THREADLIST);
        NET_THREADLIST[ClientSocket] = ((HANDLE)_beginthreadex(nullptr, 0, ThreadProcessClientSocket, (void*)&ClientSocket, 0, nullptr));
        LeaveCriticalSection(&CS_NET_THREADLIST);
    }
    closesocket(NET_SERVERSOCKET);
    EnterCriticalSection(&CS_NET_THREADLIST);
    for (auto itr = NET_THREADLIST.begin(); itr != NET_THREADLIST.end(); ++itr)
    {
        WaitForSingleObject(itr->second, INFINITE);
    }
    LeaveCriticalSection(&CS_NET_THREADLIST);
    DeleteCriticalSection(&CS_NET_THREADLIST);
    DeleteCriticalSection(&CS_NET_CLIENTLIST);
    WSACleanup();
    DB_CONN->close();
    cout << "[SYS] Server Finished!" << endl;

    return 0;
}
