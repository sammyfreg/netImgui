#include "RemoteImgui_Network.h"

#if _WIN32
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib") 

namespace RmtImgui { namespace Network {

struct SocketInfo
{
	SocketInfo(SOCKET socket) : mSocket(socket){}
	SOCKET mSocket;
};

bool Startup()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
		return false;
	
	return true;
}

void Shutdown()
{
	WSACleanup();
}

SocketInfo* Connect(const uint8_t ServerIp[4], uint32_t ServerPort)
{
	SOCKET ConnectSocket = socket(AF_INET , SOCK_STREAM , 0 );
	if(ConnectSocket == INVALID_SOCKET)
		return false;
	
	sockaddr_in server;	
	server.sin_family	= AF_INET;
	server.sin_addr		= {ServerIp[0],ServerIp[1],ServerIp[2],ServerIp[3]};
	server.sin_port		= htons( (unsigned short)ServerPort );
	if( connect(ConnectSocket , (struct sockaddr *)&server , sizeof(server)) >= 0)
		return new SocketInfo(ConnectSocket);

	return nullptr;
}

void Disconnect(SocketInfo* pClientSocket)
{
	if( pClientSocket )
	{
		closesocket(pClientSocket->mSocket);
		delete pClientSocket;
	}
}

bool DataReceive(SocketInfo* pClientSocket, void* pDataIn, size_t Size)
{
	int resultRcv = recv(pClientSocket->mSocket, reinterpret_cast<char*>(pDataIn), (int)Size, 0);	
	return resultRcv != SOCKET_ERROR && resultRcv > 0;
}

bool DataSend(SocketInfo* pClientSocket, void* pDataOut, size_t Size)
{
	int resultSend = send(pClientSocket->mSocket, reinterpret_cast<char*>(pDataOut), (int)Size, 0);
	return resultSend != SOCKET_ERROR && resultSend > 0;
}

}} // namespace RmtImgui::Network

#endif // _WIN32 ==================================================================================
