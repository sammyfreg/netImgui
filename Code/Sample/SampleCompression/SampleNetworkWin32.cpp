//=============================================================================
//@SAMPLE_EDIT
// 
// Copy of "Client\Private\NetImgui_NetworkWin32.cpp" with some minor changes 
// allowing some metrics measurements in the sample

#include "..\..\Client\Private\NetImgui_Shared.h"
#include "..\..\Client\Private\NetImgui_WarningDisableStd.h"
#include "..\..\Client\Private\NetImgui_CmdPackets.h"

uint64_t gMetric_SentDataCompressed		= 0u;
uint64_t gMetric_SentDataUncompressed	= 0u;

//#include "NetImgui_Shared.h"
//#if NETIMGUI_ENABLED && NETIMGUI_WINSOCKET_ENABLED
//
//#include "NetImgui_WarningDisableStd.h"
//=============================================================================

#include <WinSock2.h>
#include <WS2tcpip.h>

#if defined(_MSC_VER)
#pragma comment(lib, "ws2_32")
#endif


namespace NetImgui { namespace Internal { namespace Network 
{
	//=================================================================================================
	// Wrapper around native socket object and init some socket options
	//=================================================================================================
	struct SocketInfo
	{
		SocketInfo(SOCKET socket) 
		: mSocket(socket)
		{
			u_long kNonBlocking = true;
			ioctlsocket(mSocket, static_cast<long>(FIONBIO), &kNonBlocking);
		
			constexpr DWORD	kComsNoDelay = 1;
			setsockopt(mSocket, SOL_SOCKET, TCP_NODELAY, reinterpret_cast<const char*>(&kComsNoDelay), sizeof(kComsNoDelay));

			//constexpr int	kComsSendBuffer = 1014*1024;
			//setsockopt(mSocket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&kComsSendBuffer), sizeof(kComsSendBuffer));

			//constexpr int	kComsRcvBuffer = 1014*1024;
			//setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&kComsRcvBuffer), sizeof(kComsRcvBuffer));

			#if 0 // @sammyfreg : No timeout useful when debugging, to keep connection alive while code breakpoint
			constexpr DWORD	kComsTimeoutMs	= 10000;
			setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&kComsTimeoutMs), sizeof(kComsTimeoutMs));
			setsockopt(mSocket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&kComsTimeoutMs), sizeof(kComsTimeoutMs));
			#endif
		}

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

	//=================================================================================================
	// Try establishing a connection to a remote client at given address
	//=================================================================================================
	SocketInfo* Connect(const char* ServerHost, uint32_t ServerPort)
	{
		SOCKET ClientSocket = socket(AF_INET , SOCK_STREAM , 0);
		if(ClientSocket == INVALID_SOCKET)
		return nullptr;
	
		char zPortName[32]={};
		addrinfo*	pResults	= nullptr;
		SocketInfo* pSocketInfo	= nullptr;
		NetImgui::Internal::StringFormat(zPortName, "%i", ServerPort);
		getaddrinfo(ServerHost, zPortName, nullptr, &pResults);
		addrinfo*	pResultCur	= pResults;
		while( pResultCur && !pSocketInfo )
		{
			if( connect(ClientSocket, pResultCur->ai_addr, static_cast<int>(pResultCur->ai_addrlen)) == 0 )
			{
				pSocketInfo = netImguiNew<SocketInfo>(ClientSocket);
			}		
			pResultCur = pResultCur->ai_next;
		}
		freeaddrinfo(pResults);
		if( !pSocketInfo )
		{
			closesocket(ClientSocket);
		}
		return pSocketInfo;
	}

	//=================================================================================================
	// Start waiting for connection request on this socket
	//=================================================================================================
	SocketInfo* ListenStart(uint32_t ListenPort)
	{
		SOCKET ListenSocket = INVALID_SOCKET;
		if( (ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != INVALID_SOCKET )
		{
			sockaddr_in server;
			server.sin_family		= AF_INET;
			server.sin_addr.s_addr	= INADDR_ANY;
			server.sin_port			= htons(static_cast<USHORT>(ListenPort));
		
			#if NETIMGUI_FORCE_TCP_LISTEN_BINDING
			constexpr BOOL ReUseAdrValue(true);
			setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&ReUseAdrValue), sizeof(ReUseAdrValue));
			#endif
			if(	bind(ListenSocket, reinterpret_cast<sockaddr*>(&server), sizeof(server)) != SOCKET_ERROR &&
				listen(ListenSocket, 0) != SOCKET_ERROR )
			{
				u_long kIsNonBlocking = false;
				ioctlsocket(ListenSocket, static_cast<long>(FIONBIO), &kIsNonBlocking);
				return netImguiNew<SocketInfo>(ListenSocket);
			}
			closesocket(ListenSocket);
		}
		return nullptr;
	}

	//=================================================================================================
	// Establish a new connection to a remote request
	//=================================================================================================
	SocketInfo* ListenConnect(SocketInfo* ListenSocket)
	{
		if( ListenSocket )
		{
			sockaddr ClientAddress;
			int	Size(sizeof(ClientAddress));
			SOCKET ClientSocket = accept(ListenSocket->mSocket, &ClientAddress, &Size) ;
			if (ClientSocket != INVALID_SOCKET)
			{
				return netImguiNew<SocketInfo>(ClientSocket);
			}
		}
		return nullptr;
	}

	//=================================================================================================
	// Close a connection and free allocated object
	//=================================================================================================
	void Disconnect(SocketInfo* pClientSocket)
	{
		if( pClientSocket )
		{
			shutdown(pClientSocket->mSocket, SD_BOTH);
			closesocket(pClientSocket->mSocket);
			netImguiDelete(pClientSocket);
		}
	}

	//=================================================================================================
	// Return trie if data has been received, or there's a connection error
	//=================================================================================================
	bool DataReceivePending(SocketInfo* pClientSocket)
	{
		char Unused[4];
		int resultRcv = recv(pClientSocket->mSocket, Unused, 1, MSG_PEEK);
		// Note: return true on a connection error, to exit code looping on the data wait
		return resultRcv > 0 || (resultRcv == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK);
	}

	//=================================================================================================
	// Block until all requested data has been received from the remote connection
	//=================================================================================================
	bool DataReceive(SocketInfo* pClientSocket, void* pDataIn, size_t Size)
	{
		int totalRcv(0);
		while( totalRcv < static_cast<int>(Size) )
		{
			int resultRcv = recv(pClientSocket->mSocket, &reinterpret_cast<char*>(pDataIn)[totalRcv], static_cast<int>(Size)-totalRcv, 0);
			if( resultRcv != SOCKET_ERROR )
			{
				totalRcv += resultRcv;
			}
			else
			{
				if( WSAGetLastError() != WSAEWOULDBLOCK )
				{
					return false;	// Connection error, abort transmission
				}
				std::this_thread::yield();
			}
		}
		return totalRcv == static_cast<int>(Size);
	}

	//=================================================================================================
	// Block until all requested data has been sent to remote connection
	//=================================================================================================
	bool DataSend(SocketInfo* pClientSocket, void* pDataOut, size_t Size)
	{
		//=============================================================================
		//@SAMPLE_EDIT 
		// Intercept data transmission metrics used in compression sample
		auto* pCmdHeader				= reinterpret_cast<NetImgui::Internal::CmdHeader*>(pDataOut);
		gMetric_SentDataCompressed		+= Size;
		gMetric_SentDataUncompressed	+= (pCmdHeader->mType == NetImgui::Internal::CmdHeader::eCommands::DrawFrame) ?
											reinterpret_cast<const NetImgui::Internal::CmdDrawFrame*>(pDataOut)->mUncompressedSize :
											Size;
		//=============================================================================
		int totalSent(0);
		while( totalSent < static_cast<int>(Size) )
		{
			int resultSent = send(pClientSocket->mSocket, &reinterpret_cast<char*>(pDataOut)[totalSent], static_cast<int>(Size)-totalSent, 0);
			if( resultSent != SOCKET_ERROR )
			{
				totalSent += resultSent;
			}
			else
			{
				if( WSAGetLastError() != WSAEWOULDBLOCK )
				{
					return false;	// Connection error, abort transmission
				}
				std::this_thread::yield();
			}
		}
		return totalSent == static_cast<int>(Size);
	}

}}} // namespace NetImgui::Internal::Network

//=============================================================================
//@SAMPLE_EDIT
#include "..\..\Client\Private\NetImgui_WarningReenable.h"

//#include "NetImgui_WarningReenable.h"
//#else
//
// Prevents Linker warning LNK4221 in Visual Studio (This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library)
//extern int sSuppresstLNK4221_NetImgui_NetworkWin23; 
//int sSuppresstLNK4221_NetImgui_NetworkWin23(0);
//
//#endif // #if NETIMGUI_ENABLED && NETIMGUI_WINSOCKET_ENABLED
//=============================================================================
