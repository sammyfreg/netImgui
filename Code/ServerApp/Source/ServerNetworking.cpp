#include "stdafx.h"
#include "Private/NetImgui_WarningDisableStd.h"

#include <WinSock2.h>
#include <thread>

#include "ServerNetworking.h"
#include "RemoteClient.h"
#include <Private/NetImgui_Network.h>
#include <Private/NetImgui_CmdPackets.h>

namespace NetworkServer
{

static bool						gbShutdown = false;
static std::atomic_uint32_t		gActiveThreads;

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
	gActiveThreads++;
	while (bConnected && pClient->mbConnected && !gbShutdown)
	{		
		bConnected =	Communications_Outgoing(Socket, pClient) && 
						Communications_Incoming(Socket, pClient);
		Sleep(8);
	}
	closesocket(Socket);
	pClient->mName[0]		= 0;
	pClient->mbConnected	= false;
	gActiveThreads--;
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
		strncpy_s(pClient->mName, cmdVersionRcv.mClientName, sizeof(pClient->mName));
		return true;
	}
	return false;
}

//=================================================================================================
// Thread waiting on Client Connection request
//=================================================================================================
void NetworkConnectionListen(SOCKET ListenSocket, ClientRemote* pClients, uint32_t ClientCount)
{	
	while( !gbShutdown )
	{
		sockaddr	ClientAddress;
		int			Size(sizeof(ClientAddress));
		SOCKET		ClientSocket = accept(ListenSocket , &ClientAddress, &Size) ;
		if( ClientSocket != INVALID_SOCKET )
		{
			ClientRemote* pClientAvail(nullptr);
			for(uint32_t i(0); i<ClientCount && pClientAvail==nullptr; ++i)
			{
				if( pClients[i].mbConnected.exchange(true) == false)
					pClientAvail = &pClients[i];
			}

			if (pClientAvail && ClientAddress.sa_family == AF_INET)
			{
				pClientAvail->mConnectPort	= reinterpret_cast<sockaddr_in*>(&ClientAddress)->sin_port;
				pClientAvail->mConnectIP[0]	= reinterpret_cast<sockaddr_in*>(&ClientAddress)->sin_addr.S_un.S_un_b.s_b1;
				pClientAvail->mConnectIP[1]	= reinterpret_cast<sockaddr_in*>(&ClientAddress)->sin_addr.S_un.S_un_b.s_b2;
				pClientAvail->mConnectIP[2]	= reinterpret_cast<sockaddr_in*>(&ClientAddress)->sin_addr.S_un.S_un_b.s_b3;
				pClientAvail->mConnectIP[3]	= reinterpret_cast<sockaddr_in*>(&ClientAddress)->sin_addr.S_un.S_un_b.s_b4;
				pClientAvail->mName[0]		= 0;
			}
			
			const char* zErrorMsg(nullptr);
			if( pClientAvail == nullptr )
				zErrorMsg = "Too many connection on server already";

			if( zErrorMsg == nullptr && Communications_InitializeClient(ClientSocket, pClientAvail) == false )
				zErrorMsg = "Initialization failed. Wrong communication version?";
			
			if(zErrorMsg == nullptr)
			{
				std::thread(Communications_ClientExchangeLoop, ClientSocket, pClientAvail).detach();
			}
			else
			{
				if( pClientAvail )
				{
					pClientAvail->mbConnected = false;
					printf("Error connecting to client %03i.%03i.%03i.%03i:%i (%s)\n", pClientAvail->mConnectIP[3], pClientAvail->mConnectIP[2], pClientAvail->mConnectIP[1], pClientAvail->mConnectIP[0], pClientAvail->mConnectPort, zErrorMsg);
				}
				else
					printf("Error connecting to client (%s)\n", zErrorMsg);
				closesocket(ClientSocket);
			}			
		}
	}
	closesocket(ListenSocket);
}

//=================================================================================================
// Initialize Networking and start listening thread
//=================================================================================================
bool Startup(ClientRemote* pClients, uint32_t ClientCount, uint32_t ListenPort )
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

	gActiveThreads = 0;
	std::thread(NetworkConnectionListen, ListenSocket, pClients, ClientCount).detach();
	return true;
}

void Shutdown()
{
	gbShutdown = true;
	while( gActiveThreads > 0 )
		Sleep(0);
}


}
