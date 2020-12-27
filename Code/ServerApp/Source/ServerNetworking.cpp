#include "stdafx.h"
#include "Private/NetImgui_WarningDisableStd.h"

#include <thread>

#include "ServerNetworking.h"
#include "ClientRemote.h"
#include "ClientConfig.h"

#include <Private/NetImgui_Network.h>
#include <Private/NetImgui_CmdPackets.h>

namespace NetworkServer
{
static bool						gbShutdown					= false;
static bool						gbValidListenSocket			= false;
static std::atomic_uint32_t		gActiveClientThreadCount;

//=================================================================================================
//
//=================================================================================================
void Communications_Incoming_CmdTexture(ClientRemote* pClient, uint8_t*& pCmdData)
{
	if( pCmdData )
	{
		auto pCmdTexture	= reinterpret_cast<NetImgui::Internal::CmdTexture*>(pCmdData);
		pCmdData			= nullptr; // Take ownership of the data, prevent Free
		pCmdTexture->mpTextureData.ToPointer();
		pClient->ReceiveTexture(pCmdTexture);
	}
}

//=================================================================================================
//
//=================================================================================================
void Communications_Incoming_CmdDrawFrame(ClientRemote* pClient, uint8_t*& pCmdData)
{
	if( pCmdData )
	{
		auto pCmdDraw		= reinterpret_cast<NetImgui::Internal::CmdDrawFrame*>(pCmdData);
		pCmdData			= nullptr; // Take ownership of the data, prevent Free
		pCmdDraw->ToPointers();
		pClient->ReceiveDrawFrame(pCmdDraw);
	}
}

//=================================================================================================
// Receive every commands sent by remote client and process them
// We keep receiving until we detect a ping command (signal end of commands)
//=================================================================================================
bool Communications_Incoming(NetImgui::Internal::Network::SocketInfo* pClientSocket, ClientRemote* pClient)
{
	bool bOk(true);
	bool bPingReceived(false);
	NetImgui::Internal::CmdHeader cmdHeader;

	while( bOk && !bPingReceived )
	{	
		//---------------------------------------------------------------------
		// Receive all of the data from client, 
		// and allocate memory to receive it if needed
		//---------------------------------------------------------------------
		uint8_t* pCmdData	= nullptr;
		bOk					= NetImgui::Internal::Network::DataReceive(pClientSocket, &cmdHeader, sizeof(cmdHeader));
		if( bOk && cmdHeader.mSize > sizeof(cmdHeader) )
		{
			pCmdData													= NetImgui::Internal::netImguiSizedNew<uint8_t>(cmdHeader.mSize);
			*reinterpret_cast<NetImgui::Internal::CmdHeader*>(pCmdData) = cmdHeader;
			char* pDataRemaining										= reinterpret_cast<char*>(&pCmdData[sizeof(cmdHeader)]);
			const size_t sizeToRead										= cmdHeader.mSize - sizeof(cmdHeader);
			bOk															= NetImgui::Internal::Network::DataReceive(pClientSocket, pDataRemaining, sizeToRead);
		}
			
		//---------------------------------------------------------------------
		// Process the command type
		//---------------------------------------------------------------------
		if( bOk )
		{
			switch( cmdHeader.mType )
			{
			case NetImgui::Internal::CmdHeader::eCommands::Ping:		bPingReceived = true; break;
			case NetImgui::Internal::CmdHeader::eCommands::Disconnect:	bOk = false; break;
			case NetImgui::Internal::CmdHeader::eCommands::Texture:		Communications_Incoming_CmdTexture(pClient, pCmdData);		break;
			case NetImgui::Internal::CmdHeader::eCommands::DrawFrame:	Communications_Incoming_CmdDrawFrame(pClient, pCmdData);	break;
			// Commands not received in main loop, by Server
			case NetImgui::Internal::CmdHeader::eCommands::Invalid:
			case NetImgui::Internal::CmdHeader::eCommands::Version:
			case NetImgui::Internal::CmdHeader::eCommands::Input:		break;
			}
		}

		NetImgui::Internal::netImguiDeleteSafe(pCmdData);
	}

	return bOk;
}

//=================================================================================================
// Send the updates to RemoteClient
// Ends with a Ping Command (signal a end of commands)
//=================================================================================================
bool Communications_Outgoing(NetImgui::Internal::Network::SocketInfo* pClientSocket, ClientRemote* pClient)
{
	bool bSuccess(true);
	NetImgui::Internal::CmdInput* pInputCmd = pClient->CreateInputCommand();
	if( pInputCmd )
	{
		bSuccess = NetImgui::Internal::Network::DataSend(pClientSocket, reinterpret_cast<void*>(pInputCmd), pInputCmd->mHeader.mSize);
		NetImgui::Internal::netImguiDeleteSafe(pInputCmd);
	}
	
	if( pClient->mbPendingDisconnect )
	{
		NetImgui::Internal::CmdDisconnect cmdDisconnect;
		bSuccess &= NetImgui::Internal::Network::DataSend(pClientSocket, reinterpret_cast<void*>(&cmdDisconnect), cmdDisconnect.mHeader.mSize);
	}

	// Always finish with a ping
	{
		NetImgui::Internal::CmdPing cmdPing;
		bSuccess &= NetImgui::Internal::Network::DataSend(pClientSocket, reinterpret_cast<void*>(&cmdPing), cmdPing.mHeader.mSize);
	}
	return bSuccess;
}

//=================================================================================================
// Keep sending/receiving commands to Remote Client, until disconnection occurs
//=================================================================================================
void Communications_ClientExchangeLoop(NetImgui::Internal::Network::SocketInfo* pClientSocket, ClientRemote* pClient)
{	
	bool bConnected(true);
	gActiveClientThreadCount++;

	ClientConfig::SetProperty_Connected(pClient->mClientConfigID, true);
	while (bConnected && pClient->mbIsConnected && !pClient->mbPendingDisconnect && !gbShutdown)
	{		
		bConnected =	Communications_Outgoing(pClientSocket, pClient) && 
						Communications_Incoming(pClientSocket, pClient);
		std::this_thread::sleep_for(std::chrono::milliseconds(8));
	}
	ClientConfig::SetProperty_Connected(pClient->mClientConfigID, false);
	NetImgui::Internal::Network::Disconnect(pClientSocket);
	
	pClient->mInfoName[0]				= 0;
	pClient->mInfoImguiVerName[0]		= 0;
	pClient->mInfoNetImguiVerName[0]	= 0;
	pClient->mInfoImguiVerID			= 0;
	pClient->mInfoNetImguiVerID			= 0;
	pClient->mbPendingDisconnect		= false;
	pClient->mbIsConnected				= false;
	pClient->mbIsFree					= true;
	gActiveClientThreadCount--;
}

//=================================================================================================
// Establish connection with Remote Client
// Makes sure that Server/Client are compatible
//=================================================================================================
bool Communications_InitializeClient(NetImgui::Internal::Network::SocketInfo* pClientSocket, ClientRemote* pClient)
{
	NetImgui::Internal::CmdVersion cmdVersionSend;
	NetImgui::Internal::CmdVersion cmdVersionRcv;
	strcpy_s(cmdVersionSend.mClientName,"Server");
		
	if(	NetImgui::Internal::Network::DataSend(pClientSocket, reinterpret_cast<void*>(&cmdVersionSend), cmdVersionSend.mHeader.mSize) && 
		NetImgui::Internal::Network::DataReceive(pClientSocket, reinterpret_cast<void*>(&cmdVersionRcv), cmdVersionRcv.mHeader.mSize) &&
		cmdVersionRcv.mHeader.mType == NetImgui::Internal::CmdHeader::eCommands::Version && 
		cmdVersionRcv.mVersion == NetImgui::Internal::CmdVersion::eVersion::_Current )
	{		
		ClientConfig clientConfig;
		if( ClientConfig::GetConfigByID(pClient->mClientConfigID, clientConfig) )
			sprintf_s(pClient->mInfoName, "%s (%s)", clientConfig.ClientName, cmdVersionRcv.mClientName);
		else
			sprintf_s(pClient->mInfoName, "%s", cmdVersionRcv.mClientName);

		strcpy_s(pClient->mInfoImguiVerName,	cmdVersionRcv.mImguiVerName);
		strcpy_s(pClient->mInfoNetImguiVerName, cmdVersionRcv.mNetImguiVerName);
		pClient->mInfoImguiVerID	= cmdVersionRcv.mImguiVerID;
		pClient->mInfoNetImguiVerID = cmdVersionRcv.mNetImguiVerID;
		return true;
	}
	return false;
}

void NetworkConnectionNew(NetImgui::Internal::Network::SocketInfo* pClientSocket, ClientRemote* pNewClient)
{
	const char* zErrorMsg(nullptr);
	if (pNewClient == nullptr)
		zErrorMsg = "Too many connection on server already";
		
	if (zErrorMsg == nullptr && Communications_InitializeClient(pClientSocket, pNewClient) == false)
		zErrorMsg = "Initialization failed. Wrong communication version?";

	if (zErrorMsg == nullptr)
	{
		pNewClient->mbIsConnected	= true;
		pNewClient->mConnectedTime	= std::chrono::steady_clock::now();
		std::thread(Communications_ClientExchangeLoop, pClientSocket, pNewClient).detach();
	}
	else
	{
		NetImgui::Internal::Network::Disconnect(pClientSocket);
		if (pNewClient)
		{
			pNewClient->mbIsFree = true;
			printf("Error connecting to client '%s:%i' (%s)\n", pNewClient->mConnectHost, pNewClient->mConnectPort, zErrorMsg);
		}
		else
			printf("Error connecting to client (%s)\n", zErrorMsg);		
	}
}

//=================================================================================================
// Open a listening port for netImgui Client trying to connect with us
//=================================================================================================
void NetworkConnectRequest_Receive_UpdateListenSocket(NetImgui::Internal::Network::SocketInfo** ppListenSocket)
{	
	uint32_t serverPort = 0;
	while( !gbShutdown )
	{		
		if( serverPort != ClientConfig::ServerPort || *ppListenSocket == nullptr )
		{
			serverPort = ClientConfig::ServerPort;
			NetImgui::Internal::Network::Disconnect(*ppListenSocket);
			*ppListenSocket		= NetImgui::Internal::Network::ListenStart(serverPort);
			gbValidListenSocket	= *ppListenSocket != nullptr;
			if( !gbValidListenSocket )
			{
				printf("Failed to start connection listen on port : %i", serverPort);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	NetImgui::Internal::Network::Disconnect(*ppListenSocket);
}

//=================================================================================================
// Thread waiting on new Client Connection request
//=================================================================================================
void NetworkConnectRequest_Receive()
{	
	NetImgui::Internal::Network::SocketInfo* pListenSocket = nullptr;
	std::thread(NetworkConnectRequest_Receive_UpdateListenSocket, &pListenSocket).detach();

	while( !gbShutdown )
	{
		// Detect connection request from Clients
		if( pListenSocket != nullptr )
		{
			NetImgui::Internal::Network::SocketInfo* pClientSocket = NetImgui::Internal::Network::ListenConnect(pListenSocket);
			if( pClientSocket )
			{
				uint32_t freeIndex = ClientRemote::GetFreeIndex();
				if( freeIndex != ClientRemote::kInvalidClient )
				{
					ClientRemote& newClient		= ClientRemote::Get(freeIndex);
					newClient.mClientConfigID	= ClientConfig::kInvalidRuntimeID;
					NetImgui::Internal::Network::GetClientInfo(pClientSocket, newClient.mConnectHost, sizeof(newClient.mConnectHost), newClient.mConnectPort);
					NetworkConnectionNew(pClientSocket, &newClient);
				}
				else
					NetImgui::Internal::Network::Disconnect(pClientSocket);
			}
		}
		std::this_thread::yield();
	}	
}

//=================================================================================================
// Thread trying to reach out new Clients with a connection
//=================================================================================================
void NetworkConnectRequest_Send()
{		
	uint64_t loopIndex(0);
	ClientConfig clientConfig;

	while( !gbShutdown )
	{						
		uint32_t clientConfigID(ClientConfig::kInvalidRuntimeID);
		NetImgui::Internal::Network::SocketInfo* pClientSocket = nullptr;

		// Find next client configuration to attempt connection to
		uint64_t configCount	= static_cast<uint64_t>(ClientConfig::GetConfigCount());
		uint32_t configIdx		= configCount ? static_cast<uint32_t>(loopIndex++ % configCount) : 0;
		if( ClientConfig::GetConfigByIndex(configIdx, clientConfig) )
		{
			if( (clientConfig.ConnectAuto || clientConfig.ConnectRequest) && !clientConfig.Connected )
			{
				ClientConfig::SetProperty_ConnectRequest(clientConfig.RuntimeID, false);	// Reset the Connection request, we are processing it
				clientConfigID = clientConfig.RuntimeID;									// Keep track of ClientConfig we are attempting to connect to
				pClientSocket = NetImgui::Internal::Network::Connect(clientConfig.HostName, clientConfig.HostPort);
			}
		}			
	
		// Connection successful, find an available client slot
		if( pClientSocket )
		{			
			uint32_t freeIndex = ClientRemote::GetFreeIndex();
			if( freeIndex != ClientRemote::kInvalidClient )
			{			
				ClientRemote& newClient = ClientRemote::Get(freeIndex);
				if( ClientConfig::GetConfigByID(clientConfigID, clientConfig) )
				{
					strcpy_s(newClient.mInfoName, clientConfig.ClientName);
					newClient.mConnectPort		= clientConfig.HostPort;
					newClient.mClientConfigID	= clientConfigID;					
				}
				NetImgui::Internal::Network::GetClientInfo(pClientSocket, newClient.mConnectHost, sizeof(newClient.mConnectHost), newClient.mConnectPort);
				NetworkConnectionNew(pClientSocket, &newClient);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}	
}

//=================================================================================================
// Initialize Networking and start listening thread
//=================================================================================================
bool Startup( )
{	
	// Relying on shared network implementation for Winsock Init
	if( !NetImgui::Internal::Network::Startup() )
	{
		return false;
	}
	
	gActiveClientThreadCount = 0;	
	std::thread(NetworkConnectRequest_Receive).detach();
	std::thread(NetworkConnectRequest_Send).detach();
	return true;
}

void Shutdown()
{
	gbShutdown = true;
	while( gActiveClientThreadCount > 0 )
		std::this_thread::yield();
}

bool IsWaitingForConnection()
{
	return gbValidListenSocket;
}

}
