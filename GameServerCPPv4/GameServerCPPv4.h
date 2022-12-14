#pragma once
//--Define--------------------------------------------------------------------
//#define _MICRO_TEST_ENABLED
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
WSAData                      NET_WSADATA          = { 0, };
SOCKET                       NET_SERVERSOCKET     = NULL;
SOCKADDR_IN                  NET_SERVERADDR       = { 0, };
const char*                  NET_SERVER_IPV4      = "127.0.0.1";
const int                    NET_SERVER_PORT      = 5001;
const int                    NET_PACKET_SIZE      = 512;
std::map<SOCKET, ClientData> CLIENT_POOL;
CRITICAL_SECTION             CS_NETWORK_HANDLER;
//--Thread--------------------------------------------------------------------       
std::map<SOCKET, HANDLE>     THREAD_POOL;
CRITICAL_SECTION             CS_THREAD_HANDLER;
//--DBMS----------------------------------------------------------------------
const std::string            DB_SERVERNAME        = "tcp://127.0.0.1:3306";
const std::string            DB_USERNAME          = "root";
const std::string            DB_PASSWORD          = "Passw0rd";
const std::string            DB_DBNAME            = "ANIMALGUYS";
sql::Driver*                 DB_DRIVER            = nullptr;
sql::Connection*             DB_CONN              = nullptr;
sql::Statement*              DB_STMT              = nullptr;
sql::PreparedStatement*      DB_PSTMT             = nullptr;
sql::ResultSet*              DB_RS                = nullptr;
CRITICAL_SECTION             CS_DB_HANDLER;       
//--MainProgram---------------------------------------------------------------
bool                         G_PROGRAMRUNNING     = true;


/*----------------------------------------------------------------------------
    2022.12.14 이웅재
    DB처리는 하나의 드라이버에서 처리하는 게 가장 단순하고 오류가 적은 것 같다.
    다만, 오랫동안 플레이할 경우 DB_PSTMT에서 
    [Can't create more than max_prepared_stmt_count statements] 16382 
    Exception이 발생한다.
    이번 프로젝트에서는 DBCall이 많이 일어나지 않기 때문에 감수하고 진행하자.
----------------------------------------------------------------------------*/