#include "NetImgui_Shared.h"

#if NETIMGUI_ENABLED && NETIMGUI_WINSOCKET_ENABLED
#include "NetImgui_WarningDisableStd.h"
#include <WinSock2.h>
#include <WS2tcpip.h>

namespace NetImgui { namespace Internal { namespace Network 
{

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

SocketInfo* Connect(const char* ServerHost, uint32_t ServerPort)
{
	SOCKET ConnectSocket = socket(AF_INET , SOCK_STREAM , 0 );
	if(ConnectSocket == INVALID_SOCKET)
		return nullptr;
	
	char zPortName[32];
	addrinfo* pResults(nullptr);
	sprintf_s(zPortName, "%i", ServerPort);
	getaddrinfo(ServerHost, zPortName, nullptr, &pResults);
	while( pResults )
	{
		if( connect(ConnectSocket, pResults->ai_addr, static_cast<int>(pResults->ai_addrlen)) == 0 )
			return new( ImGui::MemAlloc(sizeof(SocketInfo)) ) SocketInfo(ConnectSocket);
				
		pResults = pResults->ai_next;
	}
		
	return nullptr;
}

void Disconnect(SocketInfo* pClientSocket)
{
	if( pClientSocket )
	{
		closesocket(pClientSocket->mSocket);
		ImGui::MemFree(pClientSocket);
	}
}

bool DataReceive(SocketInfo* pClientSocket, void* pDataIn, size_t Size)
{
	int resultRcv = recv(pClientSocket->mSocket, reinterpret_cast<char*>(pDataIn), static_cast<int>(Size), MSG_WAITALL);
	return resultRcv != SOCKET_ERROR && resultRcv > 0;
}

bool DataSend(SocketInfo* pClientSocket, void* pDataOut, size_t Size)
{
	int resultSend = send(pClientSocket->mSocket, reinterpret_cast<char*>(pDataOut), static_cast<int>(Size), 0);
	return resultSend != SOCKET_ERROR && resultSend > 0;
}

}}} // namespace NetImgui::Internal::Network

#include "NetImgui_WarningReenable.h"
#endif // #if NETIMGUI_ENABLED && NETIMGUI_WINSOCKET_ENABLED
