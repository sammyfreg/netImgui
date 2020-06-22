#include "NetImgui_Shared.h"

#if NETIMGUI_ENABLED && !NETIMGUI_WINSOCKET_ENABLED
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
	addrinfo* pResults(nullptr);
	sprintf(zPortName, "%i", ServerPort);
	getaddrinfo(ServerHost, zPortName, nullptr, &pResults);
	while( pResults )
	{
		if( connect(ConnectSocket, pResults->ai_addr, static_cast<int>(pResults->ai_addrlen)) == 0 )
			return netImguiNew<SocketInfo>(ConnectSocket);
				
		pResults = pResults->ai_next;
	}
		
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

#endif // #if NETIMGUI_ENABLED && !NETIMGUI_WINSOCKET_ENABLED

