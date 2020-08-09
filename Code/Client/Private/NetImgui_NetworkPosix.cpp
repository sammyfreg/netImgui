#include "NetImgui_Shared.h"

#if NETIMGUI_ENABLED && NETIMGUI_POSIX_SOCKETS_ENABLED
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

namespace NetImgui { namespace Internal { namespace Network 
{

struct SocketInfo
{
	SocketInfo(int socket) : mSocket(socket){}
	int mSocket;
};

bool Startup()
{
	return true;
}

void Shutdown()
{
}

SocketInfo* Connect(const char* ServerHost, uint32_t ServerPort)
{
	int ConnectSocket = socket(AF_INET , SOCK_STREAM , 0 );
	if(ConnectSocket == -1)
		return nullptr;
	
	char zPortName[32];
	addrinfo*	pResults	= nullptr;
	SocketInfo* pSocketInfo	= nullptr;
	sprintf(zPortName, "%i", ServerPort);
	getaddrinfo(ServerHost, zPortName, nullptr, &pResults);
	addrinfo*	pResultCur	= pResults;
	while( pResultCur && !pSocketInfo)
	{
		if( connect(ConnectSocket, pResultCur->ai_addr, static_cast<int>(pResultCur->ai_addrlen)) == 0 )
			pSocketInfo = netImguiNew<SocketInfo>(ConnectSocket);
				
		pResultCur = pResultCur->ai_next;
	}

	freeaddrinfo(pResults);
	return pSocketInfo;
}

SocketInfo* ListenStart(uint32_t ListenPort)
{
#if 0 //SF TODO (copy of Winsock at the moment)
	SOCKET ListenSocket = INVALID_SOCKET;
	if ((ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != INVALID_SOCKET)
	{
		sockaddr_in server;
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons(static_cast<USHORT>(ListenPort));
		if (bind(ListenSocket, reinterpret_cast<sockaddr*>(&server), sizeof(server)) != SOCKET_ERROR &&
			listen(ListenSocket, 0) != SOCKET_ERROR)
		{
			return netImguiNew<SocketInfo>(ListenSocket);
		}
		closesocket(ListenSocket);
	}
#endif
	return nullptr;
}

SocketInfo* ListenConnect(SocketInfo* ListenSocket)
{
#if 0 //SF TODO (copy of Winsock at the moment)
	if (ListenSocket)
	{
		sockaddr ClientAddress;
		int	Size(sizeof(ClientAddress));
		SOCKET ServerSocket = accept(ListenSocket->mSocket, &ClientAddress, &Size) ;
		if (ServerSocket != INVALID_SOCKET)
		{
			return netImguiNew<SocketInfo>(ServerSocket);
		}
	}
#endif
	return nullptr;
}

void Disconnect(SocketInfo* pClientSocket)
{
	if( pClientSocket )
	{
		close(pClientSocket->mSocket);
		netImguiDelete(pClientSocket);
	}
}

bool DataReceive(SocketInfo* pClientSocket, void* pDataIn, size_t Size)
{
	int resultRcv = recv(pClientSocket->mSocket, static_cast<char*>(pDataIn), static_cast<int>(Size), MSG_WAITALL);
	return resultRcv != -1 && resultRcv > 0;
}

bool DataSend(SocketInfo* pClientSocket, void* pDataOut, size_t Size)
{
	int resultSend = send(pClientSocket->mSocket, static_cast<char*>(pDataOut), static_cast<int>(Size), 0);
	return resultSend != -1 && resultSend > 0;
}

}}} // namespace NetImgui::Internal::Network

#endif // #if NETIMGUI_ENABLED && NETIMGUI_POSIX_SOCKETS_ENABLED

