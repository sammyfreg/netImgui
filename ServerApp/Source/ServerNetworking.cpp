#include <stdio.h>
#include <thread>
#include <winsock2.h>

#include "ServerNetworking.h"
#include "RemoteClient.h"
#include <Private/NetImGui_Network.h>
#include <Private/NetImGui_CmdPackets.h>

namespace NetworkServer
{

void Communications_Incoming_CmdTexture(void* pCmdData, ClientRemote* pClient)
{
	auto pCmdTexture = reinterpret_cast<NetImgui::Internal::CmdTexture*>(pCmdData);
	pCmdTexture->mpTextureData.ToPointer();
	pClient->ReceiveTexture(pCmdTexture);
}

void Communications_Incoming_CmdDrawFrame(void* pCmdData, ClientRemote* pClient)
{
	auto pCmdDraw = reinterpret_cast<NetImgui::Internal::CmdDrawFrame*>(pCmdData);
	pCmdDraw->ToPointers();
	pClient->ReceiveDrawFrame(pCmdDraw);
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
		char* pCmdAlloc	= nullptr;
		int sizeRead	= recv(Socket, reinterpret_cast<char*>(&cmdHeader), sizeof(cmdHeader), MSG_WAITALL);
		bOk				= sizeRead == sizeof(cmdHeader);
		if( bOk && cmdHeader.mSize > sizeof(cmdHeader) )
		{
			pCmdAlloc		= NetImgui::Internal::CastMalloc<char>(cmdHeader.mSize);
			sizeRead		= recv(Socket, &pCmdAlloc[sizeof(cmdHeader)], cmdHeader.mSize-sizeof(cmdHeader), MSG_WAITALL);
			bOk				= sizeRead == cmdHeader.mSize-sizeof(cmdHeader);
			memcpy(pCmdAlloc, &cmdHeader, sizeof(cmdHeader));
		}
			
		//---------------------------------------------------------------------
		// Process the command type
		//---------------------------------------------------------------------
		if( bOk )
		{
			switch( cmdHeader.mType )
			{
			case NetImgui::Internal::CmdHeader::kCmdPing:		
				bPingReceived = true; 
				break;
			case NetImgui::Internal::CmdHeader::kCmdDisconnect:	
				bOk = false; 
				break;
			case NetImgui::Internal::CmdHeader::kCmdTexture:	
				Communications_Incoming_CmdTexture(pCmdAlloc, pClient);
				pCmdAlloc = nullptr;	// Prevents free of data since pClient was given ownership of it
				break;
			case NetImgui::Internal::CmdHeader::kCmdDrawFrame:	
				Communications_Incoming_CmdDrawFrame(pCmdAlloc, pClient); 
				pCmdAlloc = nullptr;	// Prevents free of data since pClient was given ownership of it
				break; 
			default: break;
			}
		}

		NetImgui::Internal::SafeFree(pCmdAlloc);
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
		int result	= send(Socket, reinterpret_cast<const char*>(pInputCmd), pInputCmd->mHeader.mSize, 0);
		bSuccess	= (result == static_cast<int>(pInputCmd->mHeader.mSize));
		NetImgui::Internal::SafeFree(pInputCmd);
	}
	
	// Always finish with a ping
	{
		NetImgui::Internal::CmdPing cmdPing;
		int result = send(Socket, reinterpret_cast<const char*>(&cmdPing), cmdPing.mHeader.mSize, 0);
		bSuccess &= (result == static_cast<int>(cmdPing.mHeader.mSize));
	}
	return bSuccess;
}

//=================================================================================================
// Keep sending/receiving commands to Remote Client, until disconnection occurs
//=================================================================================================
void Communications_ClientExchangeLoop(SOCKET Socket, ClientRemote* pClient)
{	
	bool bConnected(true);
	while (bConnected && pClient->mbConnected)
	{		
		bConnected =	Communications_Outgoing(Socket, pClient) && 
						Communications_Incoming(Socket, pClient);
		Sleep(8);
	}
	closesocket(Socket);
	pClient->mName[0]		= 0;
	pClient->mbConnected	= false;
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
	int resultSend	= send(Socket, (const char*)&cmdVersionSend, cmdVersionSend.mHeader.mSize, 0);
	int resultRcv	= recv(Socket, reinterpret_cast<char*>(&cmdVersionRcv), sizeof(cmdVersionRcv), MSG_WAITALL);
	
	if(	resultSend > 0 && resultRcv > 0 && 
		cmdVersionRcv.mHeader.mType == NetImgui::Internal::CmdHeader::kCmdVersion && 
		cmdVersionRcv.mVersion == NetImgui::Internal::CmdVersion::kVer_Current )
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
	while( true )
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
				pClientAvail->mConnectPort	= ((sockaddr_in*)&ClientAddress)->sin_port;
				pClientAvail->mConnectIP[0]	= ((sockaddr_in*)&ClientAddress)->sin_addr.S_un.S_un_b.s_b1;
				pClientAvail->mConnectIP[1]	= ((sockaddr_in*)&ClientAddress)->sin_addr.S_un.S_un_b.s_b2;
				pClientAvail->mConnectIP[2]	= ((sockaddr_in*)&ClientAddress)->sin_addr.S_un.S_un_b.s_b3;
				pClientAvail->mConnectIP[3]	= ((sockaddr_in*)&ClientAddress)->sin_addr.S_un.S_un_b.s_b4;
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
	server.sin_port			= htons( (USHORT)ListenPort );
	if( bind(ListenSocket, (sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
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

	std::thread(NetworkConnectionListen, ListenSocket, pClients, ClientCount).detach();
	return true;
}

void Shutdown()
{	
}


}
