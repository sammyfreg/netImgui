#include "Private/NetImgui_WarningDisableStd.h"
#include <thread>
#include <Private/NetImgui_Network.h>
#include <Private/NetImgui_CmdPackets.h>
#include "NetImguiServer_Network.h"
#include "NetImguiServer_RemoteClient.h"
#include "NetImguiServer_Config.h"

namespace NetImguiServer { namespace Network
{

static bool						gbShutdown(false);	// Set to true when NetImguiServer exiting
static bool						gbValidListenSocket(false);
static std::atomic_uint32_t		gActiveClientThreadCount(0);
static std::atomic_uint64_t		gStatsDataSent(0);
static std::atomic_uint64_t		gStatsDataRcvd(0);

//=================================================================================================
//
//=================================================================================================
void Communications_Incoming_CmdTexture(RemoteClient::Client* pClient, uint8_t*& pCmdData)
{
	if( pCmdData )
	{
		auto pCmdTexture	= reinterpret_cast<NetImgui::Internal::CmdTexture*>(pCmdData);
		pCmdData			= nullptr; // Take ownership of the data, preventing freeing
		pCmdTexture->mpTextureData.ToPointer();
		pClient->ReceiveTexture(pCmdTexture);
	}
}

//=================================================================================================
//
//=================================================================================================
void Communications_Incoming_CmdDrawFrame(RemoteClient::Client* pClient, uint8_t*& pCmdData)
{
	if( pCmdData )
	{
		auto pCmdDraw		= reinterpret_cast<NetImgui::Internal::CmdDrawFrame*>(pCmdData);
		pCmdData			= nullptr; // Take ownership of the data, preventing freeing
		pCmdDraw->ToPointers();
		pClient->ReceiveDrawFrame(pCmdDraw);
	}
}

//=================================================================================================
// Receive every commands sent by remote client and process them
// We keep receiving until we detect a ping command (signal end of commands)
//=================================================================================================
bool Communications_Incoming(NetImgui::Internal::Network::SocketInfo* pClientSocket, RemoteClient::Client* pClient)
{
	bool bOk(true);
	bool bPingReceived(false);
	NetImgui::Internal::CmdHeader cmdHeader;
	uint64_t frameDataReceived(0);
	while( bOk && !bPingReceived )
	{	
		//---------------------------------------------------------------------
		// Receive all of the data from client, 
		// and allocate memory to receive it if needed
		//---------------------------------------------------------------------
		uint8_t* pCmdData	= nullptr;
		bOk					= NetImgui::Internal::Network::DataReceive(pClientSocket, &cmdHeader, sizeof(cmdHeader));
		frameDataReceived	+= sizeof(cmdHeader);
		if( bOk && cmdHeader.mSize > sizeof(cmdHeader) )
		{
			pCmdData													= NetImgui::Internal::netImguiSizedNew<uint8_t>(cmdHeader.mSize);
			*reinterpret_cast<NetImgui::Internal::CmdHeader*>(pCmdData) = cmdHeader;
			char* pDataRemaining										= reinterpret_cast<char*>(&pCmdData[sizeof(cmdHeader)]);
			const size_t sizeToRead										= cmdHeader.mSize - sizeof(cmdHeader);
			bOk															= NetImgui::Internal::Network::DataReceive(pClientSocket, pDataRemaining, sizeToRead);
			frameDataReceived											+= sizeToRead;
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
	pClient->mStatsDataRcvd[pClient->mStatsIndex] += frameDataReceived;
	return bOk;
}

//=================================================================================================
// Send the updates to RemoteClient
// Ends with a Ping Command (signal a end of commands)
//=================================================================================================
bool Communications_Outgoing(NetImgui::Internal::Network::SocketInfo* pClientSocket, RemoteClient::Client* pClient)
{
	bool bSuccess(true);
	NetImgui::Internal::CmdInput* pInputCmd = pClient->TakePendingInput();
	uint64_t frameDataSent(0);
	if( pInputCmd )
	{		
		bSuccess &= NetImgui::Internal::Network::DataSend(pClientSocket, reinterpret_cast<void*>(pInputCmd), pInputCmd->mHeader.mSize);
		frameDataSent += pInputCmd->mHeader.mSize;
		NetImgui::Internal::netImguiDeleteSafe(pInputCmd);
	}

	if( pClient->mbPendingDisconnect )
	{
		NetImgui::Internal::CmdDisconnect cmdDisconnect;
		bSuccess &= NetImgui::Internal::Network::DataSend(pClientSocket, reinterpret_cast<void*>(&cmdDisconnect), cmdDisconnect.mHeader.mSize);
		frameDataSent += cmdDisconnect.mHeader.mSize;
	}

	// Always finish with a ping
	{
		NetImgui::Internal::CmdPing cmdPing;
		bSuccess &= NetImgui::Internal::Network::DataSend(pClientSocket, reinterpret_cast<void*>(&cmdPing), cmdPing.mHeader.mSize);
		frameDataSent += cmdPing.mHeader.mSize;
	}
	pClient->mStatsDataSent[pClient->mStatsIndex] += frameDataSent;
	return bSuccess;
}

//=================================================================================================
// Update communications stats of a client, after a frame
//=================================================================================================
void Communications_UpdateClientStats(RemoteClient::Client& Client)
{
	// Update data transfer stats
	constexpr uint64_t kHysteresisFactor= 5; // in %
	const uint32_t idxOldest			= (Client.mStatsIndex + 1) % IM_ARRAYSIZE(Client.mStatsDataRcvd);
	const uint32_t idxNewest			= Client.mStatsIndex;
	Client.mStatsTime[idxNewest]		= std::chrono::steady_clock::now();
	auto elapsedTime					= Client.mStatsTime[idxNewest] - Client.mStatsTime[idxOldest];
	uint32_t tmMs						= static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count());
	uint64_t newDataRcvd				= Client.mStatsDataRcvd[idxNewest] - Client.mStatsDataRcvd[idxOldest];
	uint64_t newDataSent				= Client.mStatsDataSent[idxNewest] - Client.mStatsDataSent[idxOldest];
	uint32_t newDataRcvdKBs				= static_cast<uint32_t>(newDataRcvd * 1000u / 1024u / tmMs);
	uint32_t newDataSentKBs				= static_cast<uint32_t>(newDataSent * 1000u / 1024u / tmMs);
	Client.mStatsRcvdKBs				= (Client.mStatsRcvdKBs*(100u-kHysteresisFactor) + newDataRcvdKBs*kHysteresisFactor)/100u;
	Client.mStatsSentKBs				= (Client.mStatsSentKBs*(100u-kHysteresisFactor) + newDataSentKBs*kHysteresisFactor)/100u;

	// Prepate for next frame data info
	Client.mStatsIndex					= idxOldest;
	Client.mStatsDataRcvd[idxOldest]	= Client.mStatsDataRcvd[idxNewest];
	Client.mStatsDataSent[idxOldest]	= Client.mStatsDataSent[idxNewest];

	// Reset FPS to zero, when detected as not visible
	if( !Client.mbIsVisible ){
		Client.mStatsFPS				= 0.f;
	}

	gStatsDataRcvd						+= newDataRcvd;
	gStatsDataSent						+= newDataSent;
}

//=================================================================================================
// Keep sending/receiving commands to Remote Client, until disconnection occurs
//=================================================================================================
void Communications_ClientExchangeLoop(NetImgui::Internal::Network::SocketInfo* pClientSocket, RemoteClient::Client* pClient)
{	
	bool bConnected(true);
	gActiveClientThreadCount++;

	NetImguiServer::Config::Client::SetProperty_Connected(pClient->mClientConfigID, true);
	while (bConnected && !gbShutdown && pClient->mbIsConnected && !pClient->mbPendingDisconnect )
	{	
		bConnected =	Communications_Outgoing(pClientSocket, pClient) && 
						Communications_Incoming(pClientSocket, pClient);

		Communications_UpdateClientStats(*pClient);
		std::this_thread::sleep_for(std::chrono::milliseconds(8));
	}
	NetImguiServer::Config::Client::SetProperty_Connected(pClient->mClientConfigID, false);
	NetImgui::Internal::Network::Disconnect(pClientSocket);
	
	pClient->mInfoName[0]				= 0;
	pClient->mWindowID[0]				= 0;
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
bool Communications_InitializeClient(NetImgui::Internal::Network::SocketInfo* pClientSocket, RemoteClient::Client* pClient)
{
	NetImgui::Internal::CmdVersion cmdVersionSend;
	NetImgui::Internal::CmdVersion cmdVersionRcv;
	strcpy_s(cmdVersionSend.mClientName,"Server");
		
	if(	NetImgui::Internal::Network::DataSend(pClientSocket, reinterpret_cast<void*>(&cmdVersionSend), cmdVersionSend.mHeader.mSize) && 
		NetImgui::Internal::Network::DataReceive(pClientSocket, reinterpret_cast<void*>(&cmdVersionRcv), cmdVersionRcv.mHeader.mSize) &&
		cmdVersionRcv.mHeader.mType == NetImgui::Internal::CmdHeader::eCommands::Version && 
		cmdVersionRcv.mVersion == NetImgui::Internal::CmdVersion::eVersion::_Current )
	{	
		strcpy_s(pClient->mInfoName,			cmdVersionRcv.mClientName);
		strcpy_s(pClient->mInfoImguiVerName,	cmdVersionRcv.mImguiVerName);
		strcpy_s(pClient->mInfoNetImguiVerName, cmdVersionRcv.mNetImguiVerName);
		pClient->mInfoImguiVerID	= cmdVersionRcv.mImguiVerID;
		pClient->mInfoNetImguiVerID = cmdVersionRcv.mNetImguiVerID;
		pClient->mConnectedTime		= std::chrono::steady_clock::now();
		pClient->mLastUpdateTime	= std::chrono::steady_clock::now() - std::chrono::hours(1);
		pClient->mLastDrawFrame		= std::chrono::steady_clock::now();
		pClient->mStatsIndex		= 0;
		pClient->mStatsRcvdKBs		= 0;
		pClient->mStatsSentKBs		= 0;
		pClient->mStatsFPS			= 0.f;
		memset(pClient->mStatsDataRcvd, 0, sizeof(pClient->mStatsDataRcvd));
		memset(pClient->mStatsDataSent, 0, sizeof(pClient->mStatsDataSent));

		NetImguiServer::Config::Client clientConfig;		
		if( NetImguiServer::Config::Client::GetConfigByID(pClient->mClientConfigID, clientConfig) ){
			sprintf_s(pClient->mWindowID, "%s (%s)###%i", pClient->mInfoName, clientConfig.mClientName, clientConfig.mHostPort); // Using HostPort as a window unique ID
		}
		else{
			sprintf_s(pClient->mWindowID, "%s##%i", pClient->mInfoName, static_cast<int>(pClient->mClientIndex)); // Using HostPort as a window unique ID
		}
		
		return true;
	}
	return false;
}

void NetworkConnectionNew(NetImgui::Internal::Network::SocketInfo* pClientSocket, RemoteClient::Client* pNewClient)
{
	const char* zErrorMsg(nullptr);
	if (pNewClient == nullptr)
		zErrorMsg = "Too many connection on server already";
		
	if (zErrorMsg == nullptr && Communications_InitializeClient(pClientSocket, pNewClient) == false)
		zErrorMsg = "Initialization failed. Wrong communication version?";

	if (zErrorMsg == nullptr)
	{
		pNewClient->mbIsConnected	= true;		
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
		if( serverPort != NetImguiServer::Config::Server::sPort || *ppListenSocket == nullptr )
		{
			NetImgui::Internal::Network::Disconnect(*ppListenSocket);
			serverPort			= NetImguiServer::Config::Server::sPort;			
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
				uint32_t freeIndex = RemoteClient::Client::GetFreeIndex();
				if( freeIndex != RemoteClient::Client::kInvalidClient )
				{
					RemoteClient::Client& newClient		= RemoteClient::Client::Get(freeIndex);
					newClient.mClientConfigID	= NetImguiServer::Config::Client::kInvalidRuntimeID;
					newClient.mClientIndex		= freeIndex;
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
	NetImguiServer::Config::Client clientConfig;

	while( !gbShutdown )
	{						
		uint32_t clientConfigID(NetImguiServer::Config::Client::kInvalidRuntimeID);
		NetImgui::Internal::Network::SocketInfo* pClientSocket = nullptr;

		// Find next client configuration to attempt connection to
		uint64_t configCount	= static_cast<uint64_t>(NetImguiServer::Config::Client::GetConfigCount());
		uint32_t configIdx		= configCount ? static_cast<uint32_t>(loopIndex++ % configCount) : 0;
		if( NetImguiServer::Config::Client::GetConfigByIndex(configIdx, clientConfig) )
		{
			if( (clientConfig.mConnectAuto || clientConfig.mConnectRequest) && !clientConfig.mConnected && clientConfig.mHostPort != NetImguiServer::Config::Server::sPort)
			{
				NetImguiServer::Config::Client::SetProperty_ConnectRequest(clientConfig.mRuntimeID, false);	// Reset the Connection request, we are processing it
				clientConfigID	= clientConfig.mRuntimeID;									// Keep track of ClientConfig we are attempting to connect to
				pClientSocket	= NetImgui::Internal::Network::Connect(clientConfig.mHostName, clientConfig.mHostPort);
			}
		}			
	
		// Connection successful, find an available client slot
		if( pClientSocket )
		{			
			uint32_t freeIndex = RemoteClient::Client::GetFreeIndex();
			if( freeIndex != RemoteClient::Client::kInvalidClient )
			{			
				RemoteClient::Client& newClient = RemoteClient::Client::Get(freeIndex);
				if( NetImguiServer::Config::Client::GetConfigByID(clientConfigID, clientConfig) )
				{
					strcpy_s(newClient.mInfoName, clientConfig.mClientName);
					newClient.mConnectPort		= clientConfig.mHostPort;
					newClient.mClientConfigID	= clientConfigID;					
					newClient.mClientIndex		= freeIndex;
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

//=================================================================================================
// Send signal to terminate all communications and wait until all client have been released
//=================================================================================================
void Shutdown()
{
	gbShutdown = true;
	while( gActiveClientThreadCount > 0 )
		std::this_thread::yield();
}

//=================================================================================================
// True when we are listening for client's connection requests
//=================================================================================================
bool IsWaitingForConnection()
{
	return gbValidListenSocket;
}

//=================================================================================================
// Total amount of data sent to clients since start
//=================================================================================================
uint64_t GetStatsDataSent()
{
	return gStatsDataSent;

}

//=================================================================================================
// Total amount of data received from clients since start
//=================================================================================================
uint64_t GetStatsDataRcvd()
{
	return gStatsDataRcvd;
}

}} // namespace NetImguiServer { namespace Network
