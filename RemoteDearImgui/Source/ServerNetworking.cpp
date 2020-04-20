#include <stdio.h>
#include <winsock2.h>
#include "ServerNetworking.h"
#include <RemoteImgui_Network.h>
#include <RemoteImgui_CmdPackets.h>

namespace NetworkServer
{

RmtImgui::Ringbuffer<uint16_t, 1024> gPendingKeyOut;

void ProcessCmdTexture(const RmtImgui::CmdHeader& Header, void* pData, ClientInfo* pClient)
{
	MAYBE_UNUSED(Header);
	RmtImgui::CmdTexture* pCmdTexture(nullptr);
	RmtImgui::DeserializeCommand(pCmdTexture, pData);
	pClient->SetTexture(pCmdTexture);
}

void ProcessCmdDrawFrame(const RmtImgui::CmdHeader& Header, void* pData, ClientInfo* pClient)
{
	MAYBE_UNUSED(Header);
	RmtImgui::CmdDrawFrame* pCmdDraw(nullptr);
	RmtImgui::DeserializeCommand(pCmdDraw, pData);

	auto pFrameCopy = (RmtImgui::ImguiFrame*)malloc(pCmdDraw->mpFrameData->mDataSize);
	memcpy(pFrameCopy, pCmdDraw->mpFrameData, pCmdDraw->mpFrameData->mDataSize);	
	pFrameCopy->UpdatePointer();
	pClient->AddFrame(pFrameCopy);
}

bool NetworkServer_ClientInitialize(SOCKET Socket, ClientInfo* pClient)
{
	RmtImgui::CmdVersion cmdVersionSend;
	RmtImgui::CmdVersion cmdVersionRcv;
	strcpy_s(cmdVersionSend.mClientName,"Server");
	int resultSend	= send(Socket, (const char*)&cmdVersionSend, cmdVersionSend.mHeader.mSize, 0);
	int resultRcv	= recv(Socket, reinterpret_cast<char*>(&cmdVersionRcv), sizeof(cmdVersionRcv), 0);
	
	if(	resultSend > 0 && resultRcv > 0 && 
		cmdVersionRcv.mHeader.mType == RmtImgui::CmdHeader::kCmdVersion && 
		cmdVersionRcv.mVersion == RmtImgui::CmdVersion::kVer_Current )
	{
		strncpy_s(pClient->mName, cmdVersionRcv.mClientName, sizeof(pClient->mName));
		return true;
	}
	return false;
}

bool NetworkServer_ClientIncoming(SOCKET Socket, ClientInfo* pClient)
{
	bool bOk(true);
	bool bPingReceived(false);
	while( bOk && !bPingReceived )
	{	
		RmtImgui::CmdHeader	cmdHeader;
		char* pWithExtraData	= nullptr;
		int result				= recv(Socket, reinterpret_cast<char*>(&cmdHeader), sizeof(cmdHeader), 0);
		bOk						= result != SOCKET_ERROR && result > 0;
		if( bOk && cmdHeader.mSize > sizeof(RmtImgui::CmdHeader) )
		{
			pWithExtraData					= (char*)malloc(cmdHeader.mSize);
			RmtImgui::CmdHeader* pCmdHeader	= reinterpret_cast<RmtImgui::CmdHeader*>(pWithExtraData);
			*pCmdHeader						= cmdHeader;
			int resultExtra					= recv(Socket, &pWithExtraData[sizeof(RmtImgui::CmdHeader)], pCmdHeader->mSize-sizeof(RmtImgui::CmdHeader), 0);
			bOk								= resultExtra != SOCKET_ERROR && resultExtra > 0;
		}

		if( bOk )
		{
			switch( cmdHeader.mType )
			{
			case RmtImgui::CmdHeader::kCmdPing:			bPingReceived = true; break;
			case RmtImgui::CmdHeader::kCmdDisconnect:	bOk = false; break;
			case RmtImgui::CmdHeader::kCmdTexture:		ProcessCmdTexture(cmdHeader, pWithExtraData, pClient); break;
			case RmtImgui::CmdHeader::kCmdDrawFrame:	ProcessCmdDrawFrame(cmdHeader, pWithExtraData, pClient); break;
			default: break;
			}
		}

		if( pWithExtraData )
			free(pWithExtraData);
	}
	return bOk;
}

bool NetworkServer_ClientOutgoing(SOCKET Socket, ClientInfo* pClient)
{
	bool bSuccess(true);
	RmtImgui::ImguiInput* pInput = pClient->mPendingInput.Release();
	if( pInput )
	{
		RmtImgui::CmdInput cmdInput;				
		cmdInput.mInput					= *pInput;
		size_t keyCount(ARRAY_COUNT(cmdInput.mInput.KeyChars));
		gPendingKeyOut.ReadData(cmdInput.mInput.KeyChars, keyCount);
		cmdInput.mInput.KeyCharCount	= static_cast<uint8_t>(keyCount);
		SafeDelete(pInput);
		int result = send(Socket, reinterpret_cast<const char*>(&cmdInput), cmdInput.mHeader.mSize, 0);
		bSuccess &= result > 0;
	}

	// Always finish with a ping
	{
		RmtImgui::CmdPing cmdPing;
		int result = send(Socket, reinterpret_cast<const char*>(&cmdPing), cmdPing.mHeader.mSize, 0);
		bSuccess &= result > 0;
	}
	return bSuccess;
}

void NetworkServer_ClientCommunications(SOCKET Socket, ClientInfo* pClient)
{	
	bool bConnected = true;
	while (bConnected && pClient->mbConnected)
	{		
		bConnected =	NetworkServer_ClientOutgoing(Socket, pClient) && 
						NetworkServer_ClientIncoming(Socket, pClient);
		Sleep(8);
	}
	closesocket(Socket);
	pClient->mName[0]		= 0;
	pClient->mbConnected	= false;
}

void NetworkConnectionListen(SOCKET ListenSocket, ClientInfo* pClients, uint32_t ClientCount)
{	
	while( true )
	{
		sockaddr	ClientAddress;
		int			Size(sizeof(ClientAddress));
		SOCKET		ClientSocket = accept(ListenSocket , &ClientAddress, &Size) ;
		if( ClientSocket != INVALID_SOCKET )
		{
			ClientInfo* pClientAvail(nullptr);
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

			if( zErrorMsg == nullptr && NetworkServer_ClientInitialize(ClientSocket, pClientAvail) == false )
				zErrorMsg = "Initialization failed. Wrong communication version?";
			
			if(zErrorMsg == nullptr)
			{
				std::thread(NetworkServer_ClientCommunications, ClientSocket, pClientAvail).detach();
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

bool Startup(ClientInfo* pClients, uint32_t ClientCount, uint32_t ListenPort )
{	
	// Relying on shared network implementation for Winsock Init
	if( !RmtImgui::Network::Startup() )
	{
		printf("Could not initialize network library : %d\n" , WSAGetLastError());		
		return false;
	}

	SOCKET ListenSocket;
	if((ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d\n" , WSAGetLastError());		
		RmtImgui::Network::Shutdown(); 
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

ClientInfo::~ClientInfo()
{
	Reset();
}

void ClientInfo::AddFrame(RmtImgui::ImguiFrame* pFrameData)
{
	mPendingFrame.Assign(pFrameData);	
}

RmtImgui::ImguiFrame* ClientInfo::GetFrame()
{
	// Check if a new frame has been added. If yes, then take ownership of it.
	RmtImgui::ImguiFrame* pPendingFrame = mPendingFrame.Release();
	if( pPendingFrame )
	{
		SafeDelete( mpFrameDraw );
		mpFrameDraw = pPendingFrame;
	}	
	return mpFrameDraw;
}

void ClientInfo::SetTexture(const RmtImgui::CmdTexture* pTextureCmd)
{
	size_t foundIdx((size_t)-1);
	for(size_t i=0; foundIdx == -1 && i<mvTextures.size(); i++)
	{
		if( mvTextures[i].mImguiId == pTextureCmd->mTextureId )
		{
			foundIdx = i;
			dx::TextureRelease( mvTextures[foundIdx] );			
		}
	}

	if( foundIdx == -1)
	{
		foundIdx = mvTextures.size();
		mvTextures.resize(foundIdx+1);
	}	

	mvTextures[foundIdx] = dx::TextureCreate(pTextureCmd);
}

void ClientInfo::Reset()
{
	for(const auto& texHandle : mvTextures )
		dx::TextureRelease(texHandle);
	mvTextures.clear();

	mMenuId		= 0;
	mName[0]	= 0;
	mPendingFrame.Free();
	mPendingInput.Free();
	SafeDelete(mpFrameDraw);
}

void ClientInfo::UpdateInput(HWND hWindows, RmtImgui::ImguiInput& Input)
{
	RECT rect;
	POINT MousPos;
	
	RmtImgui::ImguiInput* pInputNew	= new RmtImgui::ImguiInput();
	RmtImgui::ImguiInput* pInputOld	= mPendingInput.Release();

    GetClientRect(hWindows, &rect);
	GetCursorPos(&MousPos);
	ScreenToClient(hWindows, &MousPos);

	*pInputNew = Input;
	pInputNew->ScreenSize[0]	= static_cast<uint16_t>(rect.right - rect.left);
	pInputNew->ScreenSize[1]	= static_cast<uint16_t>(rect.bottom - rect.top);
	pInputNew->MousePos[0]		= static_cast<int16_t>(MousPos.x);
	pInputNew->MousePos[1]		= static_cast<int16_t>(MousPos.y);

	uint8_t KeyStates[256];
	if( GetKeyboardState( KeyStates ) )
	{
		uint64_t Value(0);
		for(uint64_t i(0); i<ARRAY_COUNT(KeyStates); ++i)
		{
			Value |= (KeyStates[i] & 0x80) !=0 ? 1 << (i % 64) : 0;
			if( (i+1) % 64 == 0 )
			{
				pInputNew->KeysDownMask[i/64] = Value;
				Value = 0;
			}
		}
	}

	size_t keyCount(Input.KeyCharCount);
	gPendingKeyOut.AddData(Input.KeyChars, keyCount);
	Input.KeyCharCount = 0;
	SafeDelete(pInputOld);
	mPendingInput.Assign(pInputNew);
}

}
