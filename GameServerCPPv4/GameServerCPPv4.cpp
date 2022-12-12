#define  _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h> //winsock.h 위에 windows.h 선언 권장!!
#include <Windows.h>
#include <process.h>
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <string>

#include "jdbc/mysql_connection.h"
#include "jdbc/cppconn/driver.h"
#include "jdbc/cppconn/exception.h"
#include "jdbc/cppconn/prepared_statement.h"

#include "MessagePacket.h"

#pragma comment (lib, "WS2_32.lib")
#pragma comment (lib, "debug/mysqlcppconn.lib")

using namespace std;

const char* SERVER_IPV4 = "127.0.0.1";
const int SERVER_PORT = 5001;
const int PACKET_SIZE = 1024;

WSAData WSAdata = { 0, };
SOCKET ServerSocket = NULL;
SOCKADDR_IN ServerSockAddr = { 0, };

const string MYSQL_SERVERNAME = "tcp://127.0.0.1:3306";
const string MYSQL_USERNAME = "root";
const string MYSQL_PASSWORD = "Passw0rd";
const string MYSQL_DBNAME = "ANIMALGUYS";

vector<SOCKET> ClientSocketList;
CRITICAL_SECTION CS_ClientSocketList;

map<SOCKET, HANDLE> ThreadHandleList;
CRITICAL_SECTION CS_ThreadHandleList;

sql::Driver* driver = nullptr;
sql::Connection* con = nullptr;
sql::Statement* stmt = nullptr;
sql::PreparedStatement* pstmt = nullptr;
sql::ResultSet* rs = nullptr;

bool bProgramRunning = true;

template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
    size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    if (size <= 0) { throw std::runtime_error("string_format Error during formatting."); }
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

bool ProcessPacket(SOCKET ClientSocket, char* buffer)
{
    bool isS = true;
    char Packet[PACKET_SIZE] = { 0, };
    memcpy(buffer, Packet, sizeof(Packet));
    MessageHeader MsgHead = { 0, };
    memcpy(&MsgHead, Packet, sizeof(MsgHead));

    switch ((EMessageID)MsgHead.MessageID)
    {
        case EMessageID::C2S_REQ_CLIENT_LOGIN:
        {
            MessageReqClientLogin ReqMsg = { 0, };
            memcpy(&ReqMsg, buffer, sizeof(MessageReqClientLogin));

            pstmt = con->prepareStatement("SELECT * FROM PLAYER");
            rs = pstmt->executeQuery();

            string DB_PLAYER_NAME = "";
            string DB_PLAYER_ID = "";
            while (rs->next())
            {

            }
            MessageResClientLogin ResMsg = { 0, };
            ResMsg.MsgHeader.MessageID = (int)EMessageID::S2C_RES_CLIENT_LOGIN;
            ResMsg.MsgHeader.FromSocketID = ServerSocket;
            ResMsg.MsgHeader.ToSocketID = ClientSocket;
            ResMsg.MsgHeader.MessageSize = sizeof(MessageResClientLogin);
            memcpy(ResMsg.PLAYER_NAME, DB_PLAYER_NAME.c_str(), DB_PLAYER_NAME.length());

            if (send(ClientSocket,(char*)&ResMsg, sizeof(ResMsg), 0) <= 0)
            {
                isS = false;
            }
            break;
        }
    }
    return isS;
}

unsigned WINAPI ThreadRun(void* arg)
{
    SOCKET ClientSocket = *(SOCKET*)arg;

    while (bProgramRunning)
    {
        char RecvBuffer[PACKET_SIZE] = { 0, };
        int RecvBytes = recv(ClientSocket, RecvBuffer, sizeof(RecvBuffer), 0);
        if (RecvBytes <= 0)
        {
            break;
        }
        bool isValid = ProcessPacket(ClientSocket, &RecvBuffer[0]);
        if (!isValid)
        {
            break;
        }
    }
    if (bProgramRunning)
    {
        char SendBuffer[PACKET_SIZE] = { 0, };
        string strMsg = string_format("[S2C_RES_DISCONCLI] : SocketID [%lld] Disconnected!", ClientSocket);
        size_t szMsg = strMsg.length();
        memcpy(SendBuffer, strMsg.c_str(), szMsg);
        EnterCriticalSection(&CS_ClientSocketList);
        for (int i = 0; i < ClientSocketList.size(); ++i)
        {
            if (ClientSocketList[i] != ClientSocket)
            {
                send(ClientSocketList[i], SendBuffer, (int)szMsg, 0);
            }
        }
        ClientSocketList.erase(find(ClientSocketList.begin(), ClientSocketList.end(), ClientSocket));
        LeaveCriticalSection(&CS_ClientSocketList);
    }
    return 0;
}


int main(int argc, char* argv[])
{
    InitializeCriticalSection(&CS_ClientSocketList);
    InitializeCriticalSection(&CS_ThreadHandleList);

    try
    {
        driver = get_driver_instance();
        con = driver->connect(MYSQL_SERVERNAME, MYSQL_USERNAME, MYSQL_PASSWORD);
        con->setSchema(MYSQL_DBNAME);
    }
    catch (sql::SQLException e)
    {
        cout << "[ERR] SQLConnection Error Occurred. ErrorMsg : " << e.what() << endl;
        exit(-11);
    }

    
    if (WSAStartup(MAKEWORD(2, 2), &WSAdata) != 0) 
    { 
        cout << "[ERR] WSAStartup Error Occurred. ErrorCode : " << GetLastError() << endl;
        exit(-1); 
    }
    ServerSocket= socket(AF_INET, SOCK_STREAM, 0);
    if (ServerSocket == INVALID_SOCKET)
    { 
        cout << "[ERR] ServerSocket Creation Error Occurred. ErrorCode : " << GetLastError() << endl;
        exit(-2);
    }
    
    ServerSockAddr.sin_family = AF_INET; // IPV4
    ServerSockAddr.sin_addr.S_un.S_addr = inet_addr(SERVER_IPV4);
    ServerSockAddr.sin_port = htons(SERVER_PORT);
    if (::bind(ServerSocket, (SOCKADDR*)&ServerSockAddr, sizeof(ServerSockAddr)) != 0)
    {
        cout << "[ERR] ServerSocket Bind Error Occurred. ErrorCode : " << GetLastError() << endl;
        exit(-3);
    };
    if (listen(ServerSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        cout << "[ERR] ServerSocket Listen Error Occurred. ErrorCode : " << GetLastError() << endl;
        exit(-4); 
    };

    while (bProgramRunning)
    {
        SOCKADDR_IN ClientAddress = { 0, };
        int szClientAddress = sizeof(ClientAddress);
        SOCKET ClientSocket = accept(ServerSocket, (SOCKADDR*)&ClientAddress, &szClientAddress);
        if (ClientSocket == INVALID_SOCKET)
        {
            closesocket(ClientSocket);
            continue;
        }
        EnterCriticalSection(&CS_ClientSocketList);
        ClientSocketList.push_back(ClientSocket);
        MessageHeader msgHead = { 0, };
        msgHead.MessageID = (int)EMessageID::S2C_RES_CLINET_CONNECT;
        msgHead.MessageSize = sizeof(MessageHeader);
        msgHead.FromSocketID = (int)ServerSocket;
        msgHead.ToSocketID = (int)ClientSocket;
        send(ClientSocket, (char*)&msgHead, msgHead.MessageSize, 0);
        LeaveCriticalSection(&CS_ClientSocketList);
        ThreadHandleList[ClientSocket] = ((HANDLE)_beginthreadex(nullptr, 0, ThreadRun, (void*)&ClientSocket, 0, nullptr));
    }
    closesocket(ServerSocket);
    for (auto itr = ThreadHandleList.begin(); itr != ThreadHandleList.end(); ++itr)
    {
        WaitForSingleObject(itr->second, INFINITE);
    }
    DeleteCriticalSection(&CS_ClientSocketList);
    DeleteCriticalSection(&CS_ThreadHandleList);
    WSACleanup();
    con->close();

    return 0;
}
