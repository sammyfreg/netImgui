#include <thread>
#include <chrono>
#include <atomic>
#include "../NetImGui_Api.h"
#include "NetImGui_Client.h"
#include "NetImGui_Network.h"
#include "NetImGui_CmdPackets.h"

namespace NetImgui { 

Client::ClientInfo*	gpClientInfo;

void CreateNewContext()
{
	Client::ClientInfo& client = *gpClientInfo;
	if( client.mpContext )
	{
		ImGui::DestroyContext(client.mpContext);
		client.mpContext = nullptr;
	}

	client.mpContext			= ImGui::CreateContext(ImGui::GetIO().Fonts);
	ImGuiContext* pSourceCxt	= ImGui::GetCurrentContext();
	ImGuiIO& sourceIO			= ImGui::GetIO();
	ImGuiStyle& sourceStyle		= ImGui::GetStyle();
	ImGui::SetCurrentContext( client.mpContext );
	ImGuiIO& newIO				= ImGui::GetIO();
	ImGuiStyle& newStyle		= ImGui::GetStyle();

	// Import the style/options settings of current context, into this one
	newIO.ConfigFlags						= sourceIO.ConfigFlags;
	newIO.BackendFlags						= sourceIO.BackendFlags;
	newIO.IniSavingRate						= sourceIO.IniSavingRate;
	newIO.IniFilename						= sourceIO.IniFilename;
	newIO.LogFilename						= sourceIO.LogFilename;
	newIO.ConfigFlags						= sourceIO.ConfigFlags;
	newIO.MouseDoubleClickTime				= sourceIO.MouseDoubleClickTime;
	newIO.MouseDoubleClickMaxDist			= sourceIO.MouseDoubleClickMaxDist;
	newIO.MouseDragThreshold				= sourceIO.MouseDragThreshold;
	newIO.KeyRepeatDelay					= sourceIO.KeyRepeatDelay;
	newIO.KeyRepeatRate						= sourceIO.KeyRepeatRate;
	newIO.UserData							= sourceIO.UserData;
	newIO.FontGlobalScale					= sourceIO.FontGlobalScale;
	newIO.FontAllowUserScaling				= sourceIO.FontAllowUserScaling;
	newIO.DisplayFramebufferScale			= sourceIO.DisplayFramebufferScale;
	newIO.MouseDrawCursor					= false;
	newIO.ConfigMacOSXBehaviors				= sourceIO.ConfigMacOSXBehaviors;
	newIO.ConfigInputTextCursorBlink		= sourceIO.ConfigInputTextCursorBlink;
	newIO.ConfigWindowsResizeFromEdges		= sourceIO.ConfigWindowsResizeFromEdges;
	newIO.ConfigWindowsMoveFromTitleBarOnly	= sourceIO.ConfigWindowsMoveFromTitleBarOnly;
	newIO.ConfigWindowsMemoryCompactTimer	= sourceIO.ConfigWindowsMemoryCompactTimer;
	newIO.FontDefault						= sourceIO.FontDefault; // Use same FontAtlas, so pointer is valid for new context too
	memcpy(newIO.KeyMap, sourceIO.KeyMap, sizeof(newIO.KeyMap));
	memcpy(&newStyle, &sourceStyle, sizeof(newStyle));
	
	ImGui::SetCurrentContext( pSourceCxt );
}

bool Connect(const char* clientName, const uint8_t serverIp[4], uint32_t serverPort)
{
	Client::ClientInfo& client = *gpClientInfo;
	Disconnect();
	
	while( client.mbDisconnectRequest )
		std::this_thread::sleep_for(std::chrono::milliseconds(8));

	strcpy_s(client.mName, clientName);
	client.mpSocket = Network::Connect(serverIp, serverPort);
	if( client.mpSocket )
	{
		CreateNewContext();		
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

bool InputUpdateData()
{
	Client::ClientInfo& client	= *gpClientInfo;
	CmdInput* pCmdInput			= client.mPendingInputIn.Release();
	ImGuiIO& io					= ImGui::GetIO();
	if( pCmdInput )
	{		
		io.DisplaySize			= ImVec2(pCmdInput->ScreenSize[0],pCmdInput->ScreenSize[1]);
		io.MousePos				= ImVec2(pCmdInput->MousePos[0], pCmdInput->MousePos[1]);
		io.MouseWheel			= pCmdInput->MouseWheelVert - client.mMouseWheelVertPrev;
		io.MouseWheelH			= pCmdInput->MouseWheelHoriz - client.mMouseWheelHorizPrev;
		io.MouseDown[0]			= pCmdInput->IsKeyDown(CmdInput::vkMouseBtnLeft);
		io.MouseDown[1]			= pCmdInput->IsKeyDown(CmdInput::vkMouseBtnRight);
		io.MouseDown[2]			= pCmdInput->IsKeyDown(CmdInput::vkMouseBtnMid);
		io.MouseDown[3]			= pCmdInput->IsKeyDown(CmdInput::vkMouseBtnExtra1);
		io.MouseDown[4]			= pCmdInput->IsKeyDown(CmdInput::vkMouseBtnExtra2);
		io.KeyShift				= pCmdInput->IsKeyDown(CmdInput::vkKeyboardShift); 
		io.KeyCtrl				= pCmdInput->IsKeyDown(CmdInput::vkKeyboardCtrl);
		io.KeyAlt				= pCmdInput->IsKeyDown(CmdInput::vkKeyboardAlt);
		io.KeySuper				= pCmdInput->IsKeyDown(CmdInput::vkKeyboardSuper1) || pCmdInput->IsKeyDown(CmdInput::vkKeyboardSuper2);		
		//io.NavInputs //SF TODO

		memset(io.KeysDown, 0, sizeof(io.KeysDown));
		for(uint32_t i(0); i<ARRAY_COUNT(pCmdInput->KeysDownMask)*64; ++i)
			io.KeysDown[i] = (pCmdInput->KeysDownMask[i/64] & (uint64_t(1)<<(i%64))) != 0;

		//SF TODO: Optimize this
		io.ClearInputCharacters();
		size_t keyCount(1);
		uint16_t character;
		client.mPendingKeyIn.ReadData(&character, keyCount);
		while(keyCount > 0) 
		{
			io.AddInputCharacter(character);
			client.mPendingKeyIn.ReadData(&character, keyCount);
		}

		client.mMouseWheelVertPrev	= pCmdInput->MouseWheelVert;
		client.mMouseWheelHorizPrev = pCmdInput->MouseWheelHoriz;
		netImguiDeleteSafe(pCmdInput);
		return true;
	}
	return false;
}


bool NewFrame()
{
	Client::ClientInfo& client = *gpClientInfo;
	if( NetImgui::IsConnected() )
	{		
		client.mpContextRestore			= ImGui::GetCurrentContext();
		ImGui::SetCurrentContext(client.mpContext);
		if( InputUpdateData() )
		{
			static auto lastTime		= std::chrono::high_resolution_clock::now();
			auto currentTime			= std::chrono::high_resolution_clock::now();
			auto duration				= std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - lastTime);
			lastTime					= currentTime;
			ImGui::GetIO().DeltaTime	= duration.count() > 0 ? static_cast<float>(duration.count() / (1000000000.f)) : 1.f/1000.f;
			ImGui::SetCurrentContext(client.mpContext);
			ImGui::NewFrame();
			return true;
		}
		ImGui::SetCurrentContext(client.mpContextRestore);
	}
	else if( client.mpContext )
	{
		ImGui::DestroyContext(client.mpContext);
		client.mpContext = nullptr;
	}
	return false;
}

void EndFrame()
{
	Client::ClientInfo& client = *gpClientInfo;
	if( ImGui::GetCurrentContext() == client.mpContext )
	{
		uint32_t Cursor				= ImGui::GetMouseCursor();	//Must be fetched before 'Render'
		ImGui::Render();
		CmdDrawFrame* pNewDrawFrame = CreateCmdDrawDrame(ImGui::GetDrawData());
		pNewDrawFrame->mMouseCursor = Cursor;
		client.mPendingFrameOut.Assign(pNewDrawFrame);
		ImGui::SetCurrentContext(client.mpContextRestore);
		client.mpContextRestore = nullptr;
	}
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
		pCmdTexture							= netImguiNew<CmdTexture>(SizeNeeded);

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
	//SF TODO support texture removal on server
	else
	{
		pCmdTexture							= netImguiNew<CmdTexture>();
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

bool Startup()
{
	gpClientInfo = netImguiNew<Client::ClientInfo>();
	return Network::Startup();
}

void Shutdown()
{
	if( gpClientInfo )
	{
		Disconnect();
		netImguiDeleteSafe(gpClientInfo);
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
