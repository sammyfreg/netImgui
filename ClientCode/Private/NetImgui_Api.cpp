
#include <malloc.h>
#include <thread>
#include <chrono>
#include <atomic>
#include "../NetImGui_Api.h"
#include "NetImGui_Client.h"
#include "NetImGui_Network.h"
#include "NetImGui_CmdPackets.h"
#include "NetImGui_DrawFrame.h"

namespace NetImgui { 

Client::ClientInfo*	gpClientInfo;

//SF TODO: Create own context and copy/restore it instead?
bool Connect(ImGuiIO& imguiIO, const char* clientName, const uint8_t serverIp[4], uint32_t serverPort)
{
	Client::ClientInfo& client = *gpClientInfo;
	Disconnect();
	
	while( client.mbDisconnectRequest )
		std::this_thread::sleep_for(std::chrono::milliseconds(8));

	strcpy_s(client.mName, clientName);
	client.mpSocket = Network::Connect(serverIp, serverPort);
	if( client.mpSocket )
	{
		client.mpImguiIO = &imguiIO;
		std::thread(Client::Communications, &client).detach();
	}
	return client.mpSocket != nullptr;
}

void Disconnect()
{
	Client::ClientInfo& client = *gpClientInfo;
	client.mbDisconnectRequest = client.mbConnected;
}

bool IsConnected()
{
	if( gpClientInfo )
	{
		Client::ClientInfo& client = *gpClientInfo;
		return !client.mbDisconnectRequest && client.mbConnected;
	}
	return false;
}

void SendDataDraw(const ImDrawData* pImguiDrawData)
{
	Client::ClientInfo& client = *gpClientInfo;
	CmdDrawFrame* pNewDrawFrame = CreateCmdDrawDrame(pImguiDrawData);
	client.mPendingFrameOut.Assign(pNewDrawFrame);
}

void SendDataTexture(uint64_t textureId, void* pData, uint16_t width, uint16_t height, eTexFormat format)
{
	Client::ClientInfo& client = *gpClientInfo;
	CmdTexture* pCmdTexture = nullptr;
	// Add/Update a texture
	if( pData != nullptr )
	{		
		uint32_t PixelDataSize				= GetTexture_BytePerImage(format, width, height);
		uint32_t SizeNeeded					= PixelDataSize + sizeof(CmdTexture);
		pCmdTexture							= new(Malloc(SizeNeeded)) CmdTexture();

		pCmdTexture->mpTextureData.mPointer = reinterpret_cast<uint8_t*>(&pCmdTexture[1]);	
		memcpy(pCmdTexture->mpTextureData.Get(), pData, PixelDataSize);

		pCmdTexture->mHeader.mSize			= SizeNeeded;
		pCmdTexture->mWidth					= width;
		pCmdTexture->mHeight				= height;
		pCmdTexture->mTextureId				= textureId;
		pCmdTexture->mFormat				= format;
		pCmdTexture->mpTextureData.ToOffset();
	}
	// Texture to remove
	else
	{
		pCmdTexture							= new(Malloc(sizeof(CmdTexture))) CmdTexture();
		pCmdTexture->mTextureId				= textureId;
		pCmdTexture->mpTextureData.mOffset	= 0;
	}

	// In unlikely event of too many textures, wait for them to be processed 
	// (if connected) or Process them now (if not)
	while( client.mTexturesPendingCount >= ARRAY_COUNT(client.mTexturesPending) )
	{
		if( !client.mbConnected )
			client.ProcessTextures();
		else
			std::this_thread::sleep_for (std::chrono::nanoseconds(1));
	}

	uint32_t idx					= client.mTexturesPendingCount.fetch_add(1);
	client.mTexturesPending[idx]	= pCmdTexture;
	// If not connected to server, process texture immediately
	if( !client.mbConnected )
		client.ProcessTextures();

}

inline bool IsKeyPressed(const CmdInput& input, uint8_t vkKey)
{
	return (input.KeysDownMask[vkKey/64] & (uint64_t(1)<<(vkKey%64))) != 0;

}

bool InputUpdateData()
{
	Client::ClientInfo& client	= *gpClientInfo;
	CmdInput* pCmdInput			= client.mPendingInputIn.Release();
	if( pCmdInput )
	{		
		client.mpImguiIO->DisplaySize	= ImVec2(pCmdInput->ScreenSize[0],pCmdInput->ScreenSize[1]);
		client.mpImguiIO->MousePos		= ImVec2(pCmdInput->MousePos[0], pCmdInput->MousePos[1]);
		client.mpImguiIO->MouseWheel	= pCmdInput->MouseWheel;
		client.mpImguiIO->MouseDown[0]	= pCmdInput->IsKeyDown(CmdInput::vkMouseBtnLeft);
		client.mpImguiIO->MouseDown[1]	= pCmdInput->IsKeyDown(CmdInput::vkMouseBtnRight);
		client.mpImguiIO->MouseDown[2]	= pCmdInput->IsKeyDown(CmdInput::vkMouseBtnMid);
		client.mpImguiIO->KeyShift		= pCmdInput->IsKeyDown(CmdInput::vkKeyboardShift); 
		client.mpImguiIO->KeyCtrl		= pCmdInput->IsKeyDown(CmdInput::vkKeyboardCtrl);
		client.mpImguiIO->KeyAlt		= pCmdInput->IsKeyDown(CmdInput::vkKeyboardAlt);

    //bool        MouseDrawCursor;            // Request ImGui to draw a mouse cursor for you (if you are on a platform without a mouse cursor).

		memset(client.mpImguiIO->KeysDown, 0, sizeof(client.mpImguiIO->KeysDown));
		for(uint32_t i(0); i<ARRAY_COUNT(pCmdInput->KeysDownMask)*64; ++i)
			client.mpImguiIO->KeysDown[i] = (pCmdInput->KeysDownMask[i/64] & (uint64_t(1)<<(i%64))) != 0;

		//SF TODO: Optimize this
		client.mpImguiIO->ClearInputCharacters();
		size_t keyCount(1);
		do 
		{
			keyCount = 1;
			uint16_t character;
			client.mPendingKeyIn.ReadData(&character, keyCount);
			client.mpImguiIO->AddInputCharacter(character);
		}while(keyCount > 0);
		SafeFree(pCmdInput);
		return true;
	}
	return false;
}

bool Startup(void* (*pMalloc)(size_t), void (*pFree)(void*))
{
	gpMalloc		= pMalloc	? pMalloc	: &malloc;
	gpFree			= pFree		? pFree		: &free;
	gpClientInfo	= new(Malloc(sizeof(Client::ClientInfo))) Client::ClientInfo();
	return Network::Startup();
}

void Shutdown()
{
	if( gpClientInfo )
	{
		gpClientInfo->~ClientInfo();
		SafeFree(gpClientInfo);
	}
	Network::Shutdown();
}

//=============================================================================
//=============================================================================
uint8_t GetTexture_BitsPerPixel(eTexFormat eFormat)
{
	switch(eFormat)
	{
	case kTexFmtR8:		return 8*1;
	case kTexFmtRG8:	return 8*2;
	case kTexFmtRGB8:	return 8*3;
	case kTexFmtRGBA8:	return 8*4;
	default:			return 0;
	}
}

uint32_t GetTexture_BytePerLine(eTexFormat eFormat, uint32_t pixelWidth)
{		
	uint32_t bitsPerPixel = static_cast<uint32_t>(GetTexture_BitsPerPixel(eFormat));
	return pixelWidth * bitsPerPixel / 8;
	//Note: If adding support to BC compression format, have to take into account 4x4 size alignement
}
	
uint32_t GetTexture_BytePerImage(eTexFormat eFormat, uint32_t pixelWidth, uint32_t pixelHeight)
{
	return GetTexture_BytePerLine(eFormat, pixelWidth) * pixelHeight;
	//Note: If adding support to BC compression format, have to take into account 4x4 size alignement
}


} // namespace NetImgui
