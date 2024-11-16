#include "NetImgui_Shared.h"

// Tested with Unreal Engine 4.27, 5.0, 5.2

#if NETIMGUI_ENABLED && defined(__UNREAL__)

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Misc/OutputDeviceRedirector.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "HAL/PlatformProcess.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 2
#include "IPAddressAsyncResolve.h"
#endif

namespace NetImgui { namespace Internal { namespace Network 
{

//=================================================================================================
// Wrapper around native socket object and init some socket options
//=================================================================================================
struct SocketInfo
{
	SocketInfo(FSocket* pSocket) 
	: mpSocket(pSocket) 
	{
		if( mpSocket )
		{
			mpSocket->SetNonBlocking(true);
			mpSocket->SetNoDelay(true);
		}
	}

	~SocketInfo() 
	{ 
		Close(); 
	}

	void Close()
	{
		if(mpSocket )
		{
			mpSocket->Close();
			ISocketSubsystem::Get()->DestroySocket(mpSocket);
			mpSocket = nullptr;
		}
	}
	FSocket* mpSocket = nullptr;
};

bool Startup()
{
	return true;
}

void Shutdown()
{
}

//=================================================================================================
// Try establishing a connection to a remote client at given address
//=================================================================================================
SocketInfo* Connect(const char* ServerHost, uint32_t ServerPort)
{
	SocketInfo* pSocketInfo				= nullptr;
	ISocketSubsystem* SocketSubSystem	= ISocketSubsystem::Get();
	auto ResolveInfo					= SocketSubSystem->GetHostByName(ServerHost);

	while( ResolveInfo && !ResolveInfo->IsComplete() ){
		FPlatformProcess::YieldThread();
	}

	if ( ResolveInfo && ResolveInfo->GetErrorCode() == 0)
	{
		TSharedRef<FInternetAddr> IpAddress	= ResolveInfo->GetResolvedAddress().Clone();
		IpAddress->SetPort(ServerPort);
		if (IpAddress->IsValid())
		{
			FSocket* pNewSocket	= SocketSubSystem->CreateSocket(NAME_Stream, "NetImgui", IpAddress->GetProtocolType());
			if (pNewSocket)
			{
				pNewSocket->SetNonBlocking(true);
				if (pNewSocket->Connect(IpAddress.Get()))
				{
					pSocketInfo = netImguiNew<SocketInfo>(pNewSocket);
					return pSocketInfo;
				}
			}
		}		
	}
	netImguiDelete(pSocketInfo);
	return nullptr;
}

//=================================================================================================
// Start waiting for connection request on this socket
//=================================================================================================
SocketInfo* ListenStart(uint32_t ListenPort)
{
	ISocketSubsystem* PlatformSocketSub = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	TSharedPtr<FInternetAddr> IpAddress = PlatformSocketSub->GetLocalBindAddr(*GLog);
	IpAddress->SetPort(ListenPort);

	FSocket* pNewListenSocket			= PlatformSocketSub->CreateSocket(NAME_Stream, "NetImguiListen", IpAddress->GetProtocolType());
	if( pNewListenSocket )
	{
		SocketInfo* pListenSocketInfo	= netImguiNew<SocketInfo>(pNewListenSocket);
	#if NETIMGUI_FORCE_TCP_LISTEN_BINDING
		pNewListenSocket->SetReuseAddr();
	#endif
		pNewListenSocket->SetNonBlocking(false);
		pNewListenSocket->SetRecvErr();
		if (pNewListenSocket->Bind(*IpAddress))
		{		
			if (pNewListenSocket->Listen(1))
			{
				return pListenSocketInfo;
			}
		}
		netImguiDelete(pListenSocketInfo);
	}	
	return nullptr;
}

//=================================================================================================
// Establish a new connection to a remote request
//=================================================================================================
SocketInfo* ListenConnect(SocketInfo* pListenSocket)
{
	if (pListenSocket)
	{
		FSocket* pNewSocket = pListenSocket->mpSocket->Accept(FString("NetImgui"));
		if( pNewSocket )
		{
			SocketInfo* pSocketInfo = netImguiNew<SocketInfo>(pNewSocket);
			return pSocketInfo;
		}
	}
	return nullptr;
}

//=================================================================================================
// Close a connection and free allocated object
//=================================================================================================
void Disconnect(SocketInfo* pClientSocket)
{
	netImguiDelete(pClientSocket);
}

//=================================================================================================
// Return true if data has been received (or there's a connection error)
//=================================================================================================
bool DataReceivePending(SocketInfo* pClientSocket)
{
	// Note: return true on a connection error, to exit code looping on the data wait. Will handle error after DataReceive()
	uint32 PendingDataSize;
	return pClientSocket->mpSocket->HasPendingData(PendingDataSize) || (pClientSocket->mpSocket->GetConnectionState() != ESocketConnectionState::SCS_Connected);
}

//=================================================================================================
// Block until all requested data has been received from the remote connection
//=================================================================================================
bool DataReceive(SocketInfo* pClientSocket, void* pDataIn, size_t Size)
{
	int32 totalRcv(0), sizeRcv(0);
	while( totalRcv < static_cast<int>(Size) )
	{
		if( pClientSocket->mpSocket->Recv(&reinterpret_cast<uint8*>(pDataIn)[totalRcv], static_cast<int>(Size)-totalRcv, sizeRcv, ESocketReceiveFlags::None) )
		{
			totalRcv += sizeRcv;
		}
		else
		{
			if( pClientSocket->mpSocket->GetConnectionState() != ESocketConnectionState::SCS_Connected )
			{
				return false; // Connection error, abort transmission
			}
			std::this_thread::yield();
		}
	}
	return totalRcv == static_cast<int32>(Size);
}

//=================================================================================================
// Block until all requested data has been sent to remote connection
//=================================================================================================
bool DataSend(SocketInfo* pClientSocket, void* pDataOut, size_t Size)
{
	int32 totalSent(0), sizeSent(0);
	while( totalSent < static_cast<int>(Size) )
	{
		if( pClientSocket->mpSocket->Send(&reinterpret_cast<uint8*>(pDataOut)[totalSent], Size-totalSent, sizeSent) )
		{
			totalSent += sizeSent;
		}
		else
		{
			if( pClientSocket->mpSocket->GetConnectionState() != ESocketConnectionState::SCS_Connected )
			{
				return false; // Connection error, abort transmission
			}
			std::this_thread::yield();
		}
	}
	return totalSent == static_cast<int32>(Size);
}

}}} // namespace NetImgui::Internal::Network

#else

// Prevents Linker warning LNK4221 in Visual Studio (This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library)
extern int sSuppresstLNK4221_NetImgui_NetworkUE4; 
int sSuppresstLNK4221_NetImgui_NetworkUE4(0);

#endif // #if NETIMGUI_ENABLED && defined(__UNREAL__)
