#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <imgui.h>
#include "RemoteImgui_Client.h"
#include "RemoteImgui_Network.h"
#include "RemoteImgui_CmdPackets.h"
#include "RemoteImgui_ImguiFrame.h"

namespace RmtImgui { namespace Client {

struct TextureStatus
{
	~TextureStatus()
	{
		if( mpCmdTexture )
			free(mpCmdTexture);
	}
	void Set( CmdTexture* pCmdTexture )
	{
		if( mpCmdTexture )
			free(mpCmdTexture);
		mpCmdTexture	= pCmdTexture;
		mbSent			= pCmdTexture != nullptr;
	}
	bool IsValid()const
	{
		return mpCmdTexture != nullptr;
	}
	bool		mbSent		= false;
	CmdTexture* mpCmdTexture= nullptr;
};

char									gName[16];
bool									gbConnected			= false;
bool									gbDisconnectRequest	= false;
bool									gbHasNewTexture		= false;
ExchangePtr<ImguiFrame>					gPendingFrameOut;
ExchangePtr<ImguiInput>					gPendingInputIn;
std::vector<TextureStatus>				gTextures;
RmtImgui::Ringbuffer<uint16_t, 1024>	gPendingKeyIn;


//=================================================================================================
// COM INIT
//=================================================================================================
bool InitializeConnection(Network::SocketInfo* pClientSocket)
{
	CmdVersion cmdVersionSend;
	CmdVersion cmdVersionRcv;
	strcpy_s(cmdVersionSend.mClientName, gName);
	bool bResultSend	= Network::DataSend(pClientSocket, &cmdVersionSend, cmdVersionSend.mHeader.mSize);
	bool bResultRcv		= Network::DataReceive(pClientSocket, &cmdVersionRcv, sizeof(cmdVersionRcv));	
	return	bResultRcv && bResultSend && 
			cmdVersionRcv.mHeader.mType == CmdHeader::kCmdVersion && 
			cmdVersionRcv.mVersion == CmdVersion::kVer_Current;
}

//=================================================================================================
// INCOMING COMMS
//=================================================================================================
void ProcessCmdInput(const RmtImgui::CmdHeader& Header, void* pData)
{
	MAYBE_UNUSED(Header);
	RmtImgui::CmdInput* pCmdInput(nullptr);
	RmtImgui::DeserializeCommand(pCmdInput, pData);
	
	size_t keyCount(pCmdInput->mInput.KeyCharCount);
	gPendingKeyIn.AddData(pCmdInput->mInput.KeyChars, keyCount);
	ImguiInput* pNewInput	= new ImguiInput();
	*pNewInput				= pCmdInput->mInput;
	gPendingInputIn.Assign(pNewInput);	
}

bool Communications_Incoming(Network::SocketInfo* pClientSocket)
{
	bool bOk(true);
	bool bPingReceived(false);
	while( bOk && !bPingReceived )
	{
		CmdHeader cmdHeader;
		char* pExtraData	= nullptr;
		bOk					= Network::DataReceive(pClientSocket, &cmdHeader, sizeof(cmdHeader));
		if( bOk && cmdHeader.mSize > sizeof(CmdHeader) )
		{
			pExtraData		= (char*)malloc(cmdHeader.mSize);
			bOk				= Network::DataReceive(pClientSocket, &pExtraData[sizeof(cmdHeader)], cmdHeader.mSize-sizeof(cmdHeader));
		}

		if( bOk )
		{
			switch( cmdHeader.mType )
			{
			case CmdHeader::kCmdPing:		bPingReceived = true; break;
			case CmdHeader::kCmdDisconnect:	bOk = false; break;
			case CmdHeader::kCmdInput:		ProcessCmdInput(cmdHeader, pExtraData); break;
			default: break;
			}
		}
		
		if( pExtraData )
			free(pExtraData);
	}
	return bOk;
}

//=================================================================================================
// OUTGOING COMS
//=================================================================================================
bool Communications_Outgoing_Textures(Network::SocketInfo* pClientSocket)
{	
	bool bSuccess(true);
	if( gbHasNewTexture )
	{
		for(auto& cmdTexture : gTextures )
		{
			if( cmdTexture.mbSent == false )
			{
				bSuccess &= Network::DataSend(pClientSocket, cmdTexture.mpCmdTexture, cmdTexture.mpCmdTexture->mHeader.mSize);
				cmdTexture.mbSent = bSuccess;
			}
		}
		gbHasNewTexture = !bSuccess;
	}
	return bSuccess;
}

bool Communications_Outgoing_Frame(Network::SocketInfo* pClientSocket)
{
	bool bSuccess(true);
	ImguiFrame* pPendingFrame = gPendingFrameOut.Release();
	if( pPendingFrame )
	{
		CmdDrawFrame cmdDraw;
		cmdDraw.mHeader.mSize	+= pPendingFrame->mDataSize;
		//bSuccess &= Network::DataSend(pClientSocket, &cmdDraw, sizeof(cmdDraw));
		//bSuccess &= Network::DataSend(pClientSocket, pPendingFrame, pPendingFrame->mDataSize);
		char* pTempData			= new char[cmdDraw.mHeader.mSize];		//SF TODO test sending in 2 batches, without memcpy
		memcpy(pTempData, &cmdDraw, sizeof(cmdDraw));
		memcpy(pTempData+sizeof(cmdDraw), pPendingFrame, pPendingFrame->mDataSize);
		bSuccess &= Network::DataSend(pClientSocket, pTempData, cmdDraw.mHeader.mSize);
		SafeDelArray(pTempData);
		SafeDelete(pPendingFrame);
	}
	return bSuccess;
}

bool Communications_Outgoing_Disconnect(Network::SocketInfo* pClientSocket)
{
	if( gbDisconnectRequest )
	{
		CmdDisconnect cmdDisconnect;
		Network::DataSend(pClientSocket, &cmdDisconnect, cmdDisconnect.mHeader.mSize);
	}
	return true;
}

bool Communications_Outgoing_Ping(Network::SocketInfo* pClientSocket)
{
	CmdPing cmdPing;
	return Network::DataSend(pClientSocket, &cmdPing, cmdPing.mHeader.mSize);		
}

bool Communications_Outgoing(Network::SocketInfo* pClientSocket)
{		
	bool bSuccess(true);
	if( bSuccess )
		bSuccess = Communications_Outgoing_Textures(pClientSocket);
	if( bSuccess )
		bSuccess = Communications_Outgoing_Frame(pClientSocket);	
	if( bSuccess )
		bSuccess = Communications_Outgoing_Disconnect(pClientSocket);
	if( bSuccess )
		bSuccess = Communications_Outgoing_Ping(pClientSocket); // Always finish with a ping

	return bSuccess;
}

//=================================================================================================
// COMMUNICATIONS THREAD 
//=================================================================================================
void Communications(Network::SocketInfo* pClientSocket)
{	
	gbConnected	= InitializeConnection(pClientSocket);
	while( gbConnected )
	{
		if( !Communications_Outgoing(pClientSocket) || 
			!Communications_Incoming(pClientSocket) )
			gbConnected = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(8));
	}
	Network::Disconnect(pClientSocket);
	gbDisconnectRequest = false;
}

bool Connect(const char* ClientName, const uint8_t ServerIp[4], uint32_t ServerPort)
{
	Disconnect();
	while( gbDisconnectRequest )
		std::this_thread::sleep_for(std::chrono::milliseconds(8));

	strcpy_s(gName, ClientName);
	Network::SocketInfo* pClientSocket = Network::Connect(ServerIp, ServerPort);
	if( pClientSocket )
	{
		gbHasNewTexture = true;
		for(auto& cmdTexture : gTextures)
			cmdTexture.mbSent = false;
		std::thread(Communications, pClientSocket).detach();
	}
	return pClientSocket != nullptr;
}

void Disconnect()
{
	if( gbConnected )
	{
		gbDisconnectRequest = true;
	}
}

bool IsConnected()
{
	return !gbDisconnectRequest && gbConnected;
}

void SendDataFrame(const ImDrawData* pImguiDrawData)
{
	RmtImgui::ImguiFrame* pNewFrame = RmtImgui::ImguiFrame::Create(pImguiDrawData);
	gPendingFrameOut.Assign(pNewFrame);
}

void SendDataTexture(uint64_t textureId, void* pData, uint16_t width, uint16_t height, eTexFormat format)
{
	if( textureId >= gTextures.size() )
		gTextures.resize(textureId+1);

	CmdTexture cmdTexHeader;
	size_t BytePerPixel			= format == kTexFmtRGB ? 3 : 4;
	size_t PixelSize			= width * height * BytePerPixel;
	size_t SizeNeeded			= PixelSize + sizeof(CmdTexture);
	CmdTexture* pCmdTexture		= reinterpret_cast<CmdTexture*>(malloc(SizeNeeded));
	cmdTexHeader.mHeader.mSize	= (uint32_t)SizeNeeded;
	cmdTexHeader.mWidth			= width;
	cmdTexHeader.mHeight		= height;
	cmdTexHeader.mTextureId		= textureId;
	cmdTexHeader.mFormat		= format;
	*pCmdTexture				= cmdTexHeader;
	pCmdTexture->mpTextureData	= reinterpret_cast<uint8_t*>(&pCmdTexture[1]);
	memcpy(pCmdTexture->mpTextureData, pData, PixelSize);

	//SF Figure out multithread issues, delete everything on shutdown
	gTextures[textureId].Set( pCmdTexture );
	gbHasNewTexture				= true;
}

bool ReceiveDataInput(ImGuiIO& ioOut)
{
	ImguiInput* pInput = gPendingInputIn.Release();
	if( pInput )
	{
		ioOut.DisplaySize	= ImVec2(pInput->ScreenSize[0],pInput->ScreenSize[1]);
		ioOut.MousePos		= ImVec2(pInput->MousePos[0], pInput->MousePos[1]);
		ioOut.MouseDown[0]	= (pInput->MouseDownMask & ImguiInput::kMouseBtn_Left) != 0;
		ioOut.MouseDown[1]	= (pInput->MouseDownMask & ImguiInput::kMouseBtn_Right) != 0;
		ioOut.MouseDown[2]	= (pInput->MouseDownMask & ImguiInput::kMouseBtn_Middle) != 0;
		ioOut.MouseWheel	= pInput->MouseWheel;
		
		size_t keyCount(ARRAY_COUNT(ioOut.InputCharacters)-1);
		gPendingKeyIn.ReadData(ioOut.InputCharacters, keyCount);
		ioOut.InputCharacters[keyCount] = 0;

		SafeDelete(pInput);
		return true;
	}
	return false;
}

}} // namespace RmtImgui::Client