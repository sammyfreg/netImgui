#include "NetImgui_Shared.h"

#if defined(_MSC_VER) 
#pragma warning (disable: 4221)		
#endif

#if NETIMGUI_ENABLED && NETIMGUI_POSIX_SOCKETS_ENABLED
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

//NOTE: The socket handling has been modified to improve speed, but the Posix version 
//		has not been updated. Please review the changes made to 'NetImgui_NetworkWin32.cpp'
//		between version 1.11 and 1.12 and bring them over to this file. In particular :
//			-Sockets set to non blocking and immediate sending
//			-Added 'DataReceivePending' function
//			-Reworked 'DataReceive' and 'DataSend' to be non blocking socket operation
static_assert(0)

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

inline void SetNonBlocking(int Socket, bool bIsNonBlocking)
{
	int Flags	= fcntl(Socket, F_GETFL, 0);
	Flags		= bIsNonBlocking ? Flags | O_NONBLOCK : Flags ^ (Flags & O_NONBLOCK);
	fcntl(Socket, F_SETFL, Flags);
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
		{
			SetNonBlocking(ConnectSocket, false);
			pSocketInfo = netImguiNew<SocketInfo>(ConnectSocket);
		}		
		pResultCur = pResultCur->ai_next;
	}

	freeaddrinfo(pResults);
	if( !pSocketInfo )
	{
		close(ConnectSocket);
	}
	return pSocketInfo;
}

SocketInfo* ListenStart(uint32_t ListenPort)
{
	addrinfo hints;

	memset(&hints, 0, sizeof hints);
	hints.ai_family		= AF_UNSPEC;
	hints.ai_socktype	= SOCK_STREAM;
	hints.ai_flags		= AI_PASSIVE;

	addrinfo* addrInfo;
	getaddrinfo(nullptr, std::to_string(ListenPort).c_str(), &hints, &addrInfo);

	int ListenSocket = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
	if( ListenSocket != -1 )
	{
	#if NETIMGUI_FORCE_TCP_LISTEN_BINDING
		int flag = 1;
		setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
		setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag));
	#endif
		if(	bind(ListenSocket, addrInfo->ai_addr, addrInfo->ai_addrlen) != -1 &&
			listen(ListenSocket, 0) != -1)
		{
			SetNonBlocking(ListenSocket, false);
			return netImguiNew<SocketInfo>(ListenSocket);
		}
		close(ListenSocket);
	}
	return nullptr;
}

SocketInfo* ListenConnect(SocketInfo* ListenSocket)
{
	if( ListenSocket )
	{
		sockaddr_storage ClientAddress;
		socklen_t Size(sizeof(ClientAddress));
		int ServerSocket = accept(ListenSocket->mSocket, (sockaddr*)&ClientAddress, &Size) ;
		if (ServerSocket != -1)
		{
			SetNonBlocking(ServerSocket, false);
			return netImguiNew<SocketInfo>(ServerSocket);
		}
	}
	return nullptr;
}

void Disconnect(SocketInfo* pClientSocket)
{
	if( pClientSocket )
	{
		shutdown(pClientSocket->mSocket, SHUT_RDWR);
		close(pClientSocket->mSocket);
		netImguiDelete(pClientSocket);
	}
}

bool DataReceive(SocketInfo* pClientSocket, void* pDataIn, size_t Size)
{
	int resultRcv = recv(pClientSocket->mSocket, static_cast<char*>(pDataIn), static_cast<int>(Size), MSG_WAITALL);
	return static_cast<int>(Size) == resultRcv;
}

bool DataSend(SocketInfo* pClientSocket, void* pDataOut, size_t Size)
{
	int resultSend = send(pClientSocket->mSocket, static_cast<char*>(pDataOut), static_cast<int>(Size), 0);
	return static_cast<int>(Size) == resultSend;
}

}}} // namespace NetImgui::Internal::Network
#else

// Prevents Linker warning LNK4221 in Visual Studio (This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library)
extern int sSuppresstLNK4221_NetImgui_NetworkPosix; 
int sSuppresstLNK4221_NetImgui_NetworkPosix(0);

#endif // #if NETIMGUI_ENABLED && NETIMGUI_POSIX_SOCKETS_ENABLED

