#include "stdafx.h"
#include "Private/NetImgui_WarningDisableStd.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>

#include "ServerNetworking.h"
#include "ClientRemote.h"
#include "ClientConfig.h"

#include <Private/NetImgui_Network.h>
#include <Private/NetImgui_CmdPackets.h>

namespace NetworkServer
{

static constexpr DWORD			kComsTimeoutMs	= 2000;
static bool						gbShutdown		= false;
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
bool Communications_Incoming(SOCKET Socket, ClientRemote* pClient)
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
		int sizeRead	= recv(Socket, reinterpret_cast<char*>(&cmdHeader), sizeof(cmdHeader), MSG_WAITALL);
		bOk				= sizeRead == sizeof(cmdHeader);
		if( bOk && cmdHeader.mSize > sizeof(cmdHeader) )
		{
			pCmdData													= NetImgui::Internal::netImguiSizedNew<uint8_t>(cmdHeader.mSize);
			*reinterpret_cast<NetImgui::Internal::CmdHeader*>(pCmdData) = cmdHeader;
			const int sizeToRead										= static_cast<int>(cmdHeader.mSize - sizeof(cmdHeader));
			sizeRead													= recv(Socket, reinterpret_cast<char*>(&pCmdData[sizeof(cmdHeader)]), sizeToRead, MSG_WAITALL);
			bOk															= sizeRead == sizeToRead;
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
bool Communications_Outgoing(SOCKET Socket, ClientRemote* pClient)
{
	bool bSuccess(true);
	NetImgui::Internal::CmdInput* pInputCmd = pClient->CreateInputCommand();
	if( pInputCmd )
	{
		int result	= send(Socket, reinterpret_cast<const char*>(pInputCmd), static_cast<int>(pInputCmd->mHeader.mSize), 0);
		bSuccess	= (result == static_cast<int>(pInputCmd->mHeader.mSize));
		NetImgui::Internal::netImguiDeleteSafe(pInputCmd);
	}
	
	if( pClient->mbPendingDisconnect )
	{
		NetImgui::Internal::CmdDisconnect cmdDisconnect;
		int result	= send(Socket, reinterpret_cast<const char*>(&cmdDisconnect), static_cast<int>(cmdDisconnect.mHeader.mSize), 0);
		bSuccess	&= (result == static_cast<int>(cmdDisconnect.mHeader.mSize));
	}

	// Always finish with a ping
	{
		NetImgui::Internal::CmdPing cmdPing;
		int result	= send(Socket, reinterpret_cast<const char*>(&cmdPing), static_cast<int>(cmdPing.mHeader.mSize), 0);
		bSuccess	&= (result == static_cast<int>(cmdPing.mHeader.mSize));
	}
	return bSuccess;
}

//=================================================================================================
// Keep sending/receiving commands to Remote Client, until disconnection occurs
//=================================================================================================
void Communications_ClientExchangeLoop(SOCKET Socket, ClientRemote* pClient)
{	
	bool bConnected(true);
	gActiveClientThreadCount++;

	ClientConfig::SetProperty_Connected(pClient->mClientConfigID, true);
	while (bConnected && pClient->mbIsConnected && !pClient->mbPendingDisconnect && !gbShutdown)
	{		
		bConnected =	Communications_Outgoing(Socket, pClient) && 
						Communications_Incoming(Socket, pClient);
		std::this_thread::sleep_for(std::chrono::milliseconds(8));
	}
	ClientConfig::SetProperty_Connected(pClient->mClientConfigID, false);
	closesocket(Socket);
	
	pClient->mName[0]				= 0;
	pClient->mbPendingDisconnect	= false;
	pClient->mbIsConnected			= false;
	pClient->mbIsFree				= true;
	gActiveClientThreadCount--;
}

//=================================================================================================
// Establish connection with Remote Client
// Makes sure that Server/Client are compatible
//=================================================================================================
bool Communications_InitializeClient(SOCKET Socket, ClientRemote* pClient)
{
	NetImgui::Internal::CmdVersion cmdVersionSend;
	NetImgui::Internal::CmdVersion cmdVersionRcv;
	strcpy_s(cmdVersionSend.mClientName,"Server");
	int resultSend	= send(Socket, reinterpret_cast<const char*>(&cmdVersionSend), static_cast<int>(cmdVersionSend.mHeader.mSize), 0);
	int resultRcv	= recv(Socket, reinterpret_cast<char*>(&cmdVersionRcv), sizeof(cmdVersionRcv), MSG_WAITALL);
	
	if(	resultSend > 0 && resultRcv > 0 && 
		cmdVersionRcv.mHeader.mType == NetImgui::Internal::CmdHeader::eCommands::Version && 
		cmdVersionRcv.mVersion == NetImgui::Internal::CmdVersion::eVersion::_Current )
	{		
		ClientConfig clientConfig;
		if( ClientConfig::GetConfigByID(pClient->mClientConfigID, clientConfig) )
			sprintf_s(pClient->mName, "%s (%s)", clientConfig.ClientName, cmdVersionRcv.mClientName);
		else
			sprintf_s(pClient->mName, "%s", cmdVersionRcv.mClientName);
		return true;
	}
	return false;
}

void NetworkConnectionNew(SOCKET ClientSocket, ClientRemote* pNewClient)
{
	const char* zErrorMsg(nullptr);
	if (pNewClient == nullptr)
		zErrorMsg = "Too many connection on server already";

	setsockopt(ClientSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&kComsTimeoutMs), sizeof(kComsTimeoutMs));
	setsockopt(ClientSocket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&kComsTimeoutMs), sizeof(kComsTimeoutMs));
	if (zErrorMsg == nullptr && Communications_InitializeClient(ClientSocket, pNewClient) == false)
		zErrorMsg = "Initialization failed. Wrong communication version?";

	if (zErrorMsg == nullptr)
	{
		pNewClient->mbIsConnected	= true;
		pNewClient->mConnectedTime	= std::chrono::steady_clock::now();
		std::thread(Communications_ClientExchangeLoop, ClientSocket, pNewClient).detach();
	}
	else
	{
		if (pNewClient)
		{
			pNewClient->mbIsFree = true;
			printf("Error connecting to client '%s:%i' (%s)\n", pNewClient->mConnectHost, pNewClient->mConnectPort, zErrorMsg);
		}
		else
			printf("Error connecting to client (%s)\n", zErrorMsg);
		closesocket(ClientSocket);
	}
}

//=================================================================================================
// Thread waiting on new Client Connection request
//=================================================================================================
void NetworkConnectRequest_Receive(SOCKET ListenSocket)
{	
	while( !gbShutdown )
	{
		sockaddr	ClientAddress;
		int			Size(sizeof(ClientAddress));
		SOCKET		ClientSocket = accept(ListenSocket , &ClientAddress, &Size) ;
		if( ClientSocket != INVALID_SOCKET )
		{
			uint32_t freeIndex = ClientRemote::GetFreeIndex();
			if( freeIndex != ClientRemote::kInvalidClient )
			{
				char zPortBuffer[32];
				ClientRemote& newClient		= ClientRemote::Get(freeIndex);
				getnameinfo(&ClientAddress, sizeof(ClientAddress), newClient.mConnectHost, sizeof(newClient.mConnectHost), zPortBuffer, sizeof(zPortBuffer), NI_NUMERICSERV);
				newClient.mConnectPort		= atoi(zPortBuffer);
				newClient.mClientConfigID	= ClientConfig::kInvalidRuntimeID;
				NetworkConnectionNew(ClientSocket, &newClient);
			}
			else
				closesocket(ClientSocket);
		}
	}
	closesocket(ListenSocket);
}

//=================================================================================================
// Thread trying to reach out new Clients with a connection
//=================================================================================================
void NetworkConnectRequest_Send()
{		
	uint64_t loopIndex(0);
	ClientConfig clientConfig;
	SOCKET ConnectSocket(INVALID_SOCKET);
	while( !gbShutdown )
	{						
		addrinfo* pAdrResults(nullptr);
		uint32_t clientConfigID(ClientConfig::kInvalidRuntimeID);
		
		if( ConnectSocket == INVALID_SOCKET ) 	
			ConnectSocket = socket(AF_INET, SOCK_STREAM, 0);

		// Find next client configuration to attempt connection to
		uint64_t configCount	= static_cast<uint64_t>(ClientConfig::GetConfigCount());
		uint32_t configIdx		= configCount ? static_cast<uint32_t>(loopIndex++ % configCount) : 0;
		if( ClientConfig::GetConfigByIndex(configIdx, clientConfig) )
		{
			if( (clientConfig.ConnectAuto || clientConfig.ConnectRequest) && !clientConfig.Connected )
			{
				char zPortName[10];
				sprintf_s(zPortName, "%i", clientConfig.HostPort);
				getaddrinfo(clientConfig.HostName, zPortName, nullptr, &pAdrResults);
				ClientConfig::SetProperty_ConnectRequest(clientConfig.RuntimeID, false);	// Reset the Connection request, we are processing it
				clientConfigID = clientConfig.RuntimeID;									// Keep track of ClientConfig we are attempting to connect to				
			}
		}			
	
		// Attempt to reach the Client, over all resolved addresses
		bool bConnected(false);
		addrinfo* pAdrResultCur(pAdrResults);
		while( ConnectSocket != INVALID_SOCKET && pAdrResultCur && !bConnected )
		{
			bConnected		= connect(ConnectSocket, pAdrResultCur->ai_addr, static_cast<int>(pAdrResultCur->ai_addrlen)) == 0;
			pAdrResultCur	= bConnected ? pAdrResultCur : pAdrResultCur->ai_next;
		}

		// Connection successful, find an available client slot
		if( bConnected )
		{			
			uint32_t freeIndex = ClientRemote::GetFreeIndex();
			if( freeIndex != ClientRemote::kInvalidClient )
			{			
				ClientRemote& newClient = ClientRemote::Get(freeIndex);
				if( ClientConfig::GetConfigByID(clientConfigID, clientConfig) )
				{
					strcpy_s(newClient.mName, clientConfig.ClientName);
					newClient.mConnectPort		= clientConfig.HostPort;
					newClient.mClientConfigID	= clientConfigID;					
				}
				getnameinfo(pAdrResultCur->ai_addr, static_cast<socklen_t>(pAdrResultCur->ai_addrlen), newClient.mConnectHost, sizeof(newClient.mConnectHost), nullptr, 0, 0);
				NetworkConnectionNew(ConnectSocket, &newClient);
				ConnectSocket = INVALID_SOCKET;
			}
		}
		freeaddrinfo(pAdrResults);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}	
	closesocket( ConnectSocket );
}

//=================================================================================================
// Initialize Networking and start listening thread
//=================================================================================================
bool Startup( uint32_t ListenPort )
{	
	// Relying on shared network implementation for Winsock Init
	if( !NetImgui::Internal::Network::Startup() )
	{
		printf("Could not initialize network library : %d\n" , WSAGetLastError());		
		return false;
	}
	
	SOCKET ListenSocket;
	if((ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d\n" , WSAGetLastError());		
		NetImgui::Internal::Network::Shutdown(); 
		return false;
	}

	sockaddr_in server;
	server.sin_family		= AF_INET;
	server.sin_addr.s_addr	= INADDR_ANY;
	server.sin_port			= htons(static_cast<USHORT>(ListenPort) );
	if( bind(ListenSocket, reinterpret_cast<sockaddr*>(&server), sizeof(server)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		return false;
	}
	
	if( listen(ListenSocket , 3) == SOCKET_ERROR)
	{
		printf("Listen failed with error code : %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		return false;
	}

	gActiveClientThreadCount = 0;
	std::thread(NetworkConnectRequest_Receive, ListenSocket).detach();
	std::thread(NetworkConnectRequest_Send).detach();
	return true;
}

void Shutdown()
{
	gbShutdown = true;
	while( gActiveClientThreadCount > 0 )
		std::this_thread::yield();
}


}
