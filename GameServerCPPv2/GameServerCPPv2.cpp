#define _WINSOCK_DEPRECATED_NO_WARNINGS 

#include <WinSock2.h>
#include <process.h>
#include <Windows.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>

enum class MessagePacket
{
	//Server To Client
	S2C_RegisterID = 100,
	S2C_Spawn = 200,
	S2C_Destroy = 300,
	C2S_Move = 400,
	S2C_Move = 500
};

class PlayerData
{
public:
	SOCKET MySocket;
	int X;
	int Y;

	PlayerData()
	{
		MySocket = NULL;
		X = 0;
		Y = 0;
	}

	bool operator==(const PlayerData& RHS)
	{
		return MySocket == RHS.MySocket ? true : false;
	}

};

#pragma comment(lib, "WS2_32.lib")

std::map<SOCKET, PlayerData*> PlayerList;
CRITICAL_SECTION CSPlayerList;

bool G_ProgramRunning = true;

#define SERVER_IPV4 "127.0.0.1"
#define SERVER_PORT 5001

HANDLE th_ShowPlayerList = NULL;

void ProcessPacket(char* Data)
{
	unsigned short Code = 0;
	memcpy(&Code, &Data[0], sizeof(Code));
	Code = ntohs(Code);
	SOCKET FromID = 0;
	memcpy(&FromID, &Data[2], sizeof(FromID));
	FromID = ntohll(FromID);

	switch ((MessagePacket)Code)
	{
		case MessagePacket::C2S_Move:
		{
			int X = 0;
			memcpy(&X, &Data[10], sizeof(X));
			X = ntohl(X);
			int Y = 0;
			memcpy(&Y, &Data[14], sizeof(Y));
			Y = ntohl(Y);

			//Save State
			auto UpdatePlayer = PlayerList.find(FromID);
			UpdatePlayer->second->X = X;
			UpdatePlayer->second->Y = Y;

			char SendData[18] = { 0, };
			unsigned short Code = htons((unsigned short)MessagePacket::S2C_Move);
			memcpy(&Data[0], &Code, sizeof(Code));
			SOCKET SendID = htonll(FromID);
			memcpy(&Data[2], &SendID, sizeof(SendID));
			int posX = htonl(X);
			memcpy(&Data[10], &posX, sizeof(posX));
			int posY = htonl(Y);
			memcpy(&Data[14], &posY, sizeof(posY));

			for (auto Player : PlayerList)
			{
				int SendBytes = send(Player.second->MySocket, SendData, sizeof(SendData), 0);
			}
			break;
		}
		default:
		{
			std::cout << "Not Found Code." << std::endl;
			break;
		}
	}
}

unsigned WINAPI ServerSocketRecv(void* Arg)
{
	SOCKET NewClient = *(SOCKET*)Arg;
	while (true)
	{
		char data[18] = { 0, };
		int RecvLength = recv(NewClient, data, sizeof(data), 0);
		if (RecvLength <= 0)
		{
			break;
		}
		ProcessPacket(&data[0]);
	}
	closesocket(NewClient);
	EnterCriticalSection(&CSPlayerList);

	char SendBuffer[18] = { 0, };
	memset(SendBuffer, 0, sizeof(SendBuffer));
	unsigned int Code = (unsigned int)MessagePacket::S2C_Destroy;
	SOCKET SocketID = NewClient;
	memcpy(&SendBuffer[0], &Code, sizeof(Code));
	memcpy(&SendBuffer[4], &SocketID, sizeof(SocketID));
	PlayerList.erase(PlayerList.find(NewClient));
	for (auto Player : PlayerList)
	{
		int SendBytes = send(Player.second->MySocket, SendBuffer, sizeof(SendBuffer), 0);
	}
	LeaveCriticalSection(&CSPlayerList);
	return 0;
}


unsigned WINAPI ShowPlayerList(void* Arg)
{
	while (G_ProgramRunning)
	{
		WaitForSingleObject(th_ShowPlayerList, 500);
		int PlayerIndex = 0;
		EnterCriticalSection(&CSPlayerList);
		system("cls");
		for (auto Player : PlayerList)
		{
			std::cout << "[" << PlayerIndex++ << "] Socket ID : " << Player.second->MySocket
				<< ", X : " << Player.second->X
				<< ", Y : " << Player.second->Y << std::endl;
		}
		LeaveCriticalSection(&CSPlayerList);
	}
	return 0;
}


int main(int argc, char* argv[])
{
	InitializeCriticalSection(&CSPlayerList);
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { exit(-1); }
	//
	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (ListenSocket == INVALID_SOCKET) { exit(-2); }
	//
	SOCKADDR_IN ListenSocketAddr = { 0, };
	ListenSocketAddr.sin_family = PF_INET;
	ListenSocketAddr.sin_addr.S_un.S_addr = inet_addr(SERVER_IPV4);
	ListenSocketAddr.sin_port = htons(SERVER_PORT);
	//
	if (bind(ListenSocket, (SOCKADDR*)&ListenSocketAddr, sizeof(ListenSocketAddr)) != 0) { exit(-3); };
	//
	if (listen(ListenSocket, SOMAXCONN) != 0) { exit(-4); };
	//
	th_ShowPlayerList = (HANDLE)_beginthreadex(nullptr, 0, ShowPlayerList, nullptr, 0, nullptr);

	while (G_ProgramRunning)
	{
		SOCKADDR_IN NewClientAddr = { 0, };
		int NewClientAddrLength = sizeof(NewClientAddr);
		SOCKET NewClientSocket = accept(ListenSocket, (SOCKADDR*)&NewClientAddr, &NewClientAddrLength);
		EnterCriticalSection(&CSPlayerList);
		PlayerData* NewPlayer = new PlayerData();
		NewPlayer->MySocket = NewClientSocket;
		PlayerList[NewClientSocket] = NewPlayer;
		LeaveCriticalSection(&CSPlayerList);

		char SendBuffer[18] = { 0, };

		//Server->Client에게 소켓ID 전송
		//Size : 10
		memset(SendBuffer, 0, sizeof(SendBuffer));
		unsigned int Code = (unsigned int)MessagePacket::S2C_RegisterID;
		SOCKET SocketID = NewClientSocket;
		memcpy(&SendBuffer[0], &Code, sizeof(Code));
		memcpy(&SendBuffer[4], &SocketID, sizeof(SocketID));
		send(NewClientSocket, SendBuffer, sizeof(SendBuffer), 0);

		//PlayerList를 Server/Client 간 동기화하도록 로직을 구성한다.
		EnterCriticalSection(&CSPlayerList);
		//다른 클라이언트에게 새로운 클라이언트의 플레이어 정보를 전송한다.
		for (auto Player : PlayerList)
		{
			if (Player.second->MySocket != NewClientSocket)
			{
				//Size : 10
				memset(SendBuffer, 0, sizeof(SendBuffer));
				unsigned int Code = (unsigned int)MessagePacket::S2C_Spawn;
				SOCKET SocketID = NewClientSocket;
				memcpy(&SendBuffer[0], &Code, sizeof(Code));
				memcpy(&SendBuffer[4], &SocketID, sizeof(SocketID));
				send(Player.first, SendBuffer, sizeof(SendBuffer), 0);
			}
		}
		//새로운 클라이언트에게 다른 클라이언트의 플레이어 정보를 전송한다.
		for (auto Player : PlayerList)
		{
			if (Player.first != NewClientSocket)
			{
				//Size : 10
				memset(SendBuffer, 0, sizeof(SendBuffer));
				unsigned int Code = (unsigned int)MessagePacket::S2C_Spawn;
				SOCKET SocketID = Player.second->MySocket;
				memcpy(&SendBuffer[0], &Code, sizeof(Code));
				memcpy(&SendBuffer[4], &SocketID, sizeof(SocketID));
				send(NewClientSocket, SendBuffer, sizeof(SendBuffer), 0);
			}
		}
		LeaveCriticalSection(&CSPlayerList);
		HANDLE ThreadHandle = (HANDLE)_beginthreadex(nullptr, 0, ServerSocketRecv, (void*)&NewClientSocket, 0, nullptr);
	}
	closesocket(ListenSocket);
	WSACleanup();
	DeleteCriticalSection(&CSPlayerList);
}