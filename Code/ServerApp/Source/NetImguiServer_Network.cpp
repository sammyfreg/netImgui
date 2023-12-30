#include "Private/NetImgui_WarningDisableStd.h"
#include <thread>
#include <Private/NetImgui_Network.h>
#include <Private/NetImgui_CmdPackets.h>
#include "NetImguiServer_App.h"
#include "NetImguiServer_Network.h"
#include "NetImguiServer_RemoteClient.h"
#include "NetImguiServer_Config.h"

namespace NetImguiServer { namespace Network
{
using atomic_SocketInfo			= std::atomic<NetImgui::Internal::Network::SocketInfo*>;
static bool						gbShutdown(false);				// Set to true when NetImguiServer exiting
static atomic_SocketInfo		gListenSocket(nullptr);			// Need global access to kill socket on shutdown
static std::atomic_uint32_t		gActiveClientThreadCount(0);	// How many active client connection currently running
static std::atomic_bool			gActiveThreadConnectOut(false);	// True while Server is still trying to connect to new clients
static std::atomic_bool			gActiveThreadConnectIn(false);	// True while Server is still trying to receive connection from new clients
static std::atomic_uint64_t		gStatsDataSent(0);
static std::atomic_uint64_t		gStatsDataRcvd(0);

//=================================================================================================
// (IN) COMMAND TEXTURE
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
// (IN) COMMAND BACKGROUND
//=================================================================================================
void Communications_Incoming_CmdBackground(RemoteClient::Client* pClient, uint8_t*& pCmdData)
{
	if( pCmdData )
	{
		auto pCmdBackground		= reinterpret_cast<NetImgui::Internal::CmdBackground*>(pCmdData);
		pCmdData				= nullptr; // Take ownership of the data, preventing freeing
		pClient->mPendingBackgroundIn.Assign(pCmdBackground);
	}
}

//=================================================================================================
// (IN) COMMAND DRAW FRAME
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
// (IN) COMMAND CLIPBOARD
//=================================================================================================
void Communications_Incoming_CmdClipboard(RemoteClient::Client* pClient, uint8_t*& pCmdData)
{
	if( pCmdData )
	{
		auto pCmdClipboard	= reinterpret_cast<NetImgui::Internal::CmdClipboard*>(pCmdData);
		pCmdData			= nullptr; // Take ownership of the data, preventing freeing
		pCmdClipboard->ToPointers();
		pClient->mPendingClipboardIn.Assign(pCmdClipboard);
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
			case NetImgui::Internal::CmdHeader::eCommands::Background: 	Communications_Incoming_CmdBackground(pClient, pCmdData);	break;
			case NetImgui::Internal::CmdHeader::eCommands::DrawFrame:	Communications_Incoming_CmdDrawFrame(pClient, pCmdData);	break;
			case NetImgui::Internal::CmdHeader::eCommands::Clipboard:	Communications_Incoming_CmdClipboard(pClient, pCmdData);	break;
			// Commands not received in main loop, by Server
			case NetImgui::Internal::CmdHeader::eCommands::Invalid:
			case NetImgui::Internal::CmdHeader::eCommands::Version:
			case NetImgui::Internal::CmdHeader::eCommands::Input:		break;
			}
		}

		NetImgui::Internal::netImguiDeleteSafe(pCmdData);

	}
	pClient->mStatsDataRcvd += frameDataReceived;
	return bOk;
}

//=================================================================================================
// Send the updates to RemoteClient
// Ends with a Ping Command (signal a end of commands)
//=================================================================================================
bool Communications_Outgoing(NetImgui::Internal::Network::SocketInfo* pClientSocket, RemoteClient::Client* pClient)
{
	bool bSuccess(true);
	NetImgui::Internal::CmdInput* pInputCmd			= pClient->TakePendingInput();
	NetImgui::Internal::CmdClipboard* pClipboardCmd = pClient->TakePendingClipboard();
	uint64_t frameDataSent(0);
	if( pInputCmd )
	{
		bSuccess &= NetImgui::Internal::Network::DataSend(pClientSocket, reinterpret_cast<void*>(pInputCmd), pInputCmd->mHeader.mSize);
		frameDataSent += pInputCmd->mHeader.mSize;
		NetImgui::Internal::netImguiDeleteSafe(pInputCmd);
	}
	
	if( pClipboardCmd )
	{
		pClipboardCmd->ToOffsets();
		bSuccess &= NetImgui::Internal::Network::DataSend(pClientSocket, reinterpret_cast<void*>(pClipboardCmd), pClipboardCmd->mHeader.mSize);
		frameDataSent += pClipboardCmd->mHeader.mSize;
		NetImgui::Internal::netImguiDeleteSafe(pClipboardCmd);
	}

	if( pClient->mbDisconnectPending )
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
	pClient->mStatsDataSent += frameDataSent;
	return bSuccess;
}

//=================================================================================================
// Update communications stats of a client, after a frame
//=================================================================================================
void Communications_UpdateClientStats(RemoteClient::Client& Client)
{
	// Update data transfer stats
	auto elapsedTime = std::chrono::steady_clock::now() - Client.mStatsTime;	
	if( std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count() >= 250 ){
		constexpr uint64_t kHysteresis	= 10; // out of 100
		uint64_t newDataRcvd			= Client.mStatsDataRcvd - Client.mStatsDataRcvdPrev;
		uint64_t newDataSent			= Client.mStatsDataSent - Client.mStatsDataSentPrev;
		uint64_t tmMicrosS				= std::chrono::duration_cast<std::chrono::microseconds>(elapsedTime).count();
		uint32_t newDataRcvdBps			= static_cast<uint32_t>(newDataRcvd * 1000000u / tmMicrosS);
		uint32_t newDataSentBps			= static_cast<uint32_t>(newDataSent * 1000000u / tmMicrosS);
		Client.mStatsRcvdBps			= (Client.mStatsRcvdBps*(100u-kHysteresis) + newDataRcvdBps*kHysteresis)/100u;
		Client.mStatsSentBps			= (Client.mStatsSentBps*(100u-kHysteresis) + newDataSentBps*kHysteresis)/100u;
		gStatsDataRcvd					+= newDataRcvd;
		gStatsDataSent					+= newDataSent;

		Client.mStatsTime				= std::chrono::steady_clock::now();
		Client.mStatsDataRcvdPrev		= Client.mStatsDataRcvd;
		Client.mStatsDataSentPrev		= Client.mStatsDataSent;
	}
	
	// Reset FPS to zero, when detected as not visible
	if( !Client.mbIsVisible ){
		Client.mStatsFPS = 0.f;
	}
}

//=================================================================================================
// Keep sending/receiving commands to Remote Client, until disconnection occurs
//=================================================================================================
void Communications_ClientExchangeLoop(NetImgui::Internal::Network::SocketInfo* pClientSocket, RemoteClient::Client* pClient)
{	
	bool bConnected(true);
	gActiveClientThreadCount++;

	NetImguiServer::Config::Client::SetProperty_Connected(pClient->mClientConfigID, true);
	while (bConnected && !gbShutdown && pClient->mbIsConnected && !pClient->mbDisconnectPending )
	{	
		bConnected =	Communications_Outgoing(pClientSocket, pClient) && 
						Communications_Incoming(pClientSocket, pClient);

		Communications_UpdateClientStats(*pClient);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	NetImguiServer::Config::Client::SetProperty_Connected(pClient->mClientConfigID, false);
	NetImgui::Internal::Network::Disconnect(pClientSocket);
	
	pClient->Release();
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
	NetImgui::Internal::StringCopy(cmdVersionSend.mClientName, "Server");
		
	if(	NetImgui::Internal::Network::DataSend(pClientSocket, reinterpret_cast<void*>(&cmdVersionSend), cmdVersionSend.mHeader.mSize) && 
		NetImgui::Internal::Network::DataReceive(pClientSocket, reinterpret_cast<void*>(&cmdVersionRcv), cmdVersionRcv.mHeader.mSize) &&
		cmdVersionRcv.mHeader.mType == NetImgui::Internal::CmdHeader::eCommands::Version && 
		cmdVersionRcv.mVersion == NetImgui::Internal::CmdVersion::eVersion::_current )
	{			
		pClient->Initialize();
		pClient->mInfoImguiVerID	= cmdVersionRcv.mImguiVerID;
		pClient->mInfoNetImguiVerID = cmdVersionRcv.mNetImguiVerID;
		NetImgui::Internal::StringCopy(pClient->mInfoName,				cmdVersionRcv.mClientName);
		NetImgui::Internal::StringCopy(pClient->mInfoImguiVerName,		cmdVersionRcv.mImguiVerName);
		NetImgui::Internal::StringCopy(pClient->mInfoNetImguiVerName,	cmdVersionRcv.mNetImguiVerName);

		NetImguiServer::Config::Client clientConfig;
		if( NetImguiServer::Config::Client::GetConfigByID(pClient->mClientConfigID, clientConfig) ){
			NetImgui::Internal::StringFormat(pClient->mWindowID, "%s (%s)##%i", pClient->mInfoName, clientConfig.mClientName, static_cast<int>(pClient->mClientIndex)); // Using ClientIndex as a window unique ID
		}
		else{
			NetImgui::Internal::StringFormat(pClient->mWindowID, "%s##%i", pClient->mInfoName, static_cast<int>(pClient->mClientIndex)); // Using ClientIndex as a window unique ID
		}
		
		return true;
	}
	return false;
}

void NetworkConnectionNew(NetImgui::Internal::Network::SocketInfo* pClientSocket, RemoteClient::Client* pNewClient)
{
	const char* zErrorMsg(nullptr);
	if (pNewClient == nullptr) {
		zErrorMsg = "Too many connection on server already";
	}
		
	if (zErrorMsg == nullptr && !gbShutdown && Communications_InitializeClient(pClientSocket, pNewClient) == false) {
		zErrorMsg = "Initialization failed. Wrong communication version?";
	}

	if (zErrorMsg == nullptr && !gbShutdown){
		pNewClient->mbIsConnected = true;
		std::thread(Communications_ClientExchangeLoop, pClientSocket, pNewClient).detach();
	}
	else{
		NetImgui::Internal::Network::Disconnect(pClientSocket);
		if (!gbShutdown) {
			if (pNewClient){
				pNewClient->mbIsFree = true;
				printf("Error connecting to client '%s:%i' (%s)\n", pNewClient->mConnectHost, pNewClient->mConnectPort, zErrorMsg);
			}
			else {
				printf("Error connecting to client (%s)\n", zErrorMsg);
			}
		}
	}
}

//=================================================================================================
// Thread waiting on new Client Connection request
//=================================================================================================
void NetworkConnectRequest_Receive()
{	
	uint32_t serverPort(0);
	
	gActiveThreadConnectIn = true;
	while( !gbShutdown )
	{
		// Open (and update when needed) listening socket
		if (gListenSocket == nullptr || serverPort != NetImguiServer::Config::Server::sPort)
		{
			serverPort		= NetImguiServer::Config::Server::sPort;
			gListenSocket	= NetImgui::Internal::Network::ListenStart(serverPort);
			if( gListenSocket.load() != nullptr )
			{
				printf("Failed to start connection listen on port : %i", serverPort);
				std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Reduce Server listening socket open attempt frequency
			}
		}

		// Detect connection request from Clients
		if( gListenSocket.load() != nullptr )
		{
			NetImgui::Internal::Network::SocketInfo* pClientSocket = NetImgui::Internal::Network::ListenConnect(gListenSocket.load());
			if( pClientSocket )
			{
				uint32_t freeIndex = RemoteClient::Client::GetFreeIndex();
				if( freeIndex != RemoteClient::Client::kInvalidClient )
				{
					RemoteClient::Client& newClient	= RemoteClient::Client::Get(freeIndex);
					newClient.mClientConfigID		= NetImguiServer::Config::Client::kInvalidRuntimeID;
					newClient.mClientIndex			= freeIndex;
					NetImguiServer::App::HAL_GetSocketInfo(pClientSocket, newClient.mConnectHost, sizeof(newClient.mConnectHost), newClient.mConnectPort);
					NetworkConnectionNew(pClientSocket, &newClient);
				}
				else
					NetImgui::Internal::Network::Disconnect(pClientSocket);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
	NetImgui::Internal::Network::SocketInfo* socketDisconnect = gListenSocket.exchange(nullptr);
	NetImgui::Internal::Network::Disconnect(socketDisconnect);
	gActiveThreadConnectIn = false;
}

//=================================================================================================
// Thread trying to reach out new Clients with a connection
//=================================================================================================
void NetworkConnectRequest_Send()
{		
	uint64_t loopIndex(0);
	NetImguiServer::Config::Client clientConfig;
	gActiveThreadConnectOut = true;
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
				clientConfigID	= clientConfig.mRuntimeID;													// Keep track of ClientConfig we are attempting to connect to
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
					NetImgui::Internal::StringCopy(newClient.mInfoName, clientConfig.mClientName);
					newClient.mConnectPort		= clientConfig.mHostPort;
					newClient.mClientConfigID	= clientConfigID;
					newClient.mClientIndex		= freeIndex;
				}
				NetImguiServer::App::HAL_GetSocketInfo(pClientSocket, newClient.mConnectHost, sizeof(newClient.mConnectHost), newClient.mConnectPort);
				NetworkConnectionNew(pClientSocket, &newClient);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	gActiveThreadConnectOut = false;
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
	
	gbShutdown = false;
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
	NetImgui::Internal::Network::SocketInfo* socketDisconnect = gListenSocket.exchange(nullptr);
	NetImgui::Internal::Network::Disconnect(socketDisconnect);
	while( gActiveClientThreadCount > 0 || gActiveThreadConnectIn || gActiveThreadConnectOut )
		std::this_thread::yield();

	NetImgui::Internal::Network::Shutdown();
}

//=================================================================================================
// True when we are listening for client's connection requests
//=================================================================================================
bool IsWaitingForConnection()
{
	return gListenSocket.load() != nullptr;
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
