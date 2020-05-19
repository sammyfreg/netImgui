#include "NetImgui_Shared.h"
#include "NetImgui_WarningDisable.h"

#if NETIMGUI_ENABLED
#include "NetImgui_Client.h"
#include "NetImgui_Network.h"
#include "NetImgui_CmdPackets.h"

using namespace NetImgui::Internal;

namespace NetImgui { 

static Client::ClientInfo* gpClientInfo = nullptr;

void CreateImguiContext();
bool InputUpdateData();

//=================================================================================================
//
//=================================================================================================
void DefaultStartCommunicationThread(void ComFunctPtr(void*), void* pClient)
{
	std::thread(ComFunctPtr, pClient).detach();
}

bool Connect(const char* clientName, const char* ServerHost, uint32_t serverPort)
{
	return Connect(DefaultStartCommunicationThread, clientName, ServerHost, serverPort);
}

bool Connect(StartThreadFunctPtr startThreadFunction, const char* clientName, const char* ServerHost, uint32_t serverPort)
{
	Client::ClientInfo& client	= *gpClientInfo;	
	Disconnect();
	
	while( client.mbDisconnectRequest )
		std::this_thread::sleep_for(std::chrono::milliseconds(8));

	strcpy_s(client.mName, clientName);
	client.mpSocket = Network::Connect(ServerHost, serverPort);
	if( client.mpSocket )
	{
		CreateImguiContext();		
		startThreadFunction(Client::Communications, &client);		
	}

	client.mbConnectRequest = client.mpSocket != nullptr;
	return client.mbConnectRequest;
}

//=================================================================================================
//
//=================================================================================================
void Disconnect()
{
	Client::ClientInfo& client	= *gpClientInfo;
	client.mbDisconnectRequest	= client.mbConnected;
	client.mbConnectRequest		= false;
}

//=================================================================================================
//
//=================================================================================================
bool IsConnected()
{
	if( gpClientInfo )
	{
		Client::ClientInfo& client = *gpClientInfo;
		return !client.mbDisconnectRequest && client.mbConnected;
	}
	return false;
}

//=================================================================================================
//
//=================================================================================================
bool IsRemoteDraw()
{
	Client::ClientInfo& client	= *gpClientInfo;
	return ImGui::GetCurrentContext() == client.mpContext;
}

//=================================================================================================
//
//=================================================================================================
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
	else if( !client.mbConnectRequest && client.mpContext )
	{
		ImGui::DestroyContext(client.mpContext);
		client.mpContext = nullptr;
	}
	return false;
}

//=================================================================================================
//! @note:	Be carefull with the returned value, the pointer remain valid only as long as
//!			a new imgui frame hasn't been started for the netImgui remote app
//=================================================================================================
const ImDrawData* EndFrame()
{
	Client::ClientInfo& client	= *gpClientInfo;
	const ImDrawData* pDraw		= nullptr;
	if( ImGui::GetCurrentContext() == client.mpContext )
	{
		ImGuiMouseCursor Cursor		= ImGui::GetMouseCursor();	//Must be fetched before 'Render'
		ImGui::Render();
		pDraw						= ImGui::GetDrawData();
		CmdDrawFrame* pNewDrawFrame = CreateCmdDrawDrame(pDraw, Cursor);
		
		client.mPendingFrameOut.Assign(pNewDrawFrame);
		ImGui::SetCurrentContext(client.mpContextRestore);
		client.mpContextRestore = nullptr;
	}
	return pDraw;
}

//=================================================================================================
//
//=================================================================================================
void SendDataTexture(uint64_t textureId, void* pData, uint16_t width, uint16_t height, eTexFormat format)
{
	Client::ClientInfo& client				= *gpClientInfo;
	CmdTexture* pCmdTexture					= nullptr;
	
	// Add/Update a texture
	if( pData != nullptr )
	{		
		uint32_t PixelDataSize				= GetTexture_BytePerImage(format, width, height);
		uint32_t SizeNeeded					= PixelDataSize + sizeof(CmdTexture);
		pCmdTexture							= netImguiNew<CmdTexture>(SizeNeeded);

		pCmdTexture->mpTextureData.SetPtr(reinterpret_cast<uint8_t*>(&pCmdTexture[1]));
		memcpy(pCmdTexture->mpTextureData.Get(), pData, PixelDataSize);

		pCmdTexture->mHeader.mSize			= SizeNeeded;
		pCmdTexture->mWidth					= width;
		pCmdTexture->mHeight				= height;
		pCmdTexture->mTextureId				= textureId;
		pCmdTexture->mFormat				= static_cast<uint8_t>(format);
		pCmdTexture->mpTextureData.ToOffset();
	}
	// Texture to remove
	else
	{
		pCmdTexture							= netImguiNew<CmdTexture>();
		pCmdTexture->mTextureId				= textureId;		
		pCmdTexture->mWidth					= 0;
		pCmdTexture->mHeight				= 0;
		pCmdTexture->mFormat				= kTexFmt_Invalid;
		pCmdTexture->mpTextureData.SetOff(0);
	}

	// In unlikely event of too many textures, wait for them to be processed 
	// (if connected) or Process them now (if not)
	while( client.mTexturesPendingCount >= static_cast<int32_t>(ArrayCount(client.mTexturesPending)) )
	{
		if( IsConnected() )
			std::this_thread::sleep_for (std::chrono::nanoseconds(1));
		else
			client.TextureProcessPending();
	}
	int32_t idx						= client.mTexturesPendingCount.fetch_add(1);
	client.mTexturesPending[idx]	= pCmdTexture;

	// If not connected to server yet, update all pending textures
	if( !IsConnected() )
		client.TextureProcessPending();
}

//=================================================================================================
//
//=================================================================================================
bool Startup()
{
	gpClientInfo = netImguiNew<Client::ClientInfo>();
	return Network::Startup();
}

//=================================================================================================
//
//=================================================================================================
void Shutdown()
{
	Disconnect();
	while( gpClientInfo->mbConnected )
		std::this_thread::sleep_for (std::chrono::nanoseconds(1));
	Network::Shutdown();

	for( auto& texture : gpClientInfo->mTextures )
		texture.Set(nullptr);

	netImguiDeleteSafe(gpClientInfo);		
}

//=================================================================================================
//
//=================================================================================================
ImGuiContext* GetRemoteContext()
{
	Client::ClientInfo& client = *gpClientInfo;
	return client.mpContext;
}

//=================================================================================================
//
//=================================================================================================
uint8_t GetTexture_BitsPerPixel(eTexFormat eFormat)
{
	switch(eFormat)
	{
	//case kTexFmtR8:		return 8*1;
	//case kTexFmtRG8:		return 8*2;
	//case kTexFmtRGB8:		return 8*3;
	case kTexFmtRGBA8:		return 8*4;
	case kTexFmt_Invalid:	return 0;
	}
	return 0;
}

//=================================================================================================
//
//=================================================================================================
uint32_t GetTexture_BytePerLine(eTexFormat eFormat, uint32_t pixelWidth)
{		
	uint32_t bitsPerPixel = static_cast<uint32_t>(GetTexture_BitsPerPixel(eFormat));
	return pixelWidth * bitsPerPixel / 8;
	//Note: If adding support to BC compression format, have to take into account 4x4 size alignement
}
	
//=================================================================================================
//
//=================================================================================================
uint32_t GetTexture_BytePerImage(eTexFormat eFormat, uint32_t pixelWidth, uint32_t pixelHeight)
{
	return GetTexture_BytePerLine(eFormat, pixelWidth) * pixelHeight;
	//Note: If adding support to BC compression format, have to take into account 4x4 size alignement
}

//=================================================================================================
//
//=================================================================================================
void CreateImguiContext()
{
	Client::ClientInfo& client = *gpClientInfo;
	if (client.mpContext)
	{
		ImGui::DestroyContext(client.mpContext);
		client.mpContext = nullptr;
	}

	client.mpContext						= ImGui::CreateContext(ImGui::GetIO().Fonts);
	ImGuiContext* pSourceCxt				= ImGui::GetCurrentContext();
	ImGuiIO& sourceIO						= ImGui::GetIO();
	ImGuiStyle& sourceStyle					= ImGui::GetStyle();
	ImGui::SetCurrentContext(client.mpContext);
	ImGuiIO& newIO							= ImGui::GetIO();
	ImGuiStyle& newStyle					= ImGui::GetStyle();

	memcpy(newIO.KeyMap, sourceIO.KeyMap, sizeof(newIO.KeyMap));
	memcpy(&newStyle, &sourceStyle, sizeof(newStyle));

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
	newIO.ConfigMacOSXBehaviors				= sourceIO.ConfigMacOSXBehaviors;
	newIO.ConfigInputTextCursorBlink		= sourceIO.ConfigInputTextCursorBlink;
	newIO.ConfigWindowsResizeFromEdges		= sourceIO.ConfigWindowsResizeFromEdges;
	newIO.ConfigWindowsMoveFromTitleBarOnly	= sourceIO.ConfigWindowsMoveFromTitleBarOnly;
	newIO.ConfigWindowsMemoryCompactTimer	= sourceIO.ConfigWindowsMemoryCompactTimer;
	newIO.FontDefault						= sourceIO.FontDefault; // Use same FontAtlas, so pointer is valid for new context too
	newIO.MouseDrawCursor					= false;

	ImGui::SetCurrentContext(pSourceCxt);
}

//=================================================================================================
//
//=================================================================================================
bool InputUpdateData()
{
	Client::ClientInfo& client = *gpClientInfo;
	CmdInput* pCmdInput = client.mPendingInputIn.Release();
	ImGuiIO& io = ImGui::GetIO();
	if (pCmdInput)
	{
		io.DisplaySize = ImVec2(pCmdInput->mScreenSize[0], pCmdInput->mScreenSize[1]);
		io.MousePos = ImVec2(pCmdInput->mMousePos[0], pCmdInput->mMousePos[1]);
		io.MouseWheel = pCmdInput->mMouseWheelVert - client.mMouseWheelVertPrev;
		io.MouseWheelH = pCmdInput->mMouseWheelHoriz - client.mMouseWheelHorizPrev;
		io.MouseDown[0] = pCmdInput->IsKeyDown(CmdInput::vkMouseBtnLeft);
		io.MouseDown[1] = pCmdInput->IsKeyDown(CmdInput::vkMouseBtnRight);
		io.MouseDown[2] = pCmdInput->IsKeyDown(CmdInput::vkMouseBtnMid);
		io.MouseDown[3] = pCmdInput->IsKeyDown(CmdInput::vkMouseBtnExtra1);
		io.MouseDown[4] = pCmdInput->IsKeyDown(CmdInput::vkMouseBtnExtra2);
		io.KeyShift = pCmdInput->IsKeyDown(CmdInput::vkKeyboardShift);
		io.KeyCtrl = pCmdInput->IsKeyDown(CmdInput::vkKeyboardCtrl);
		io.KeyAlt = pCmdInput->IsKeyDown(CmdInput::vkKeyboardAlt);
		io.KeySuper = pCmdInput->IsKeyDown(CmdInput::vkKeyboardSuper1) || pCmdInput->IsKeyDown(CmdInput::vkKeyboardSuper2);
		//io.NavInputs //SF TODO

		memset(io.KeysDown, 0, sizeof(io.KeysDown));
		for (uint32_t i(0); i < ArrayCount(pCmdInput->mKeysDownMask) * 64; ++i)
			io.KeysDown[i] = (pCmdInput->mKeysDownMask[i / 64] & (uint64_t(1) << (i % 64))) != 0;

		//SF TODO: Optimize this
		io.ClearInputCharacters();
		size_t keyCount(1);
		uint16_t character;
		client.mPendingKeyIn.ReadData(&character, keyCount);
		while (keyCount > 0)
		{
			io.AddInputCharacter(character);
			client.mPendingKeyIn.ReadData(&character, keyCount);
		}

		client.mMouseWheelVertPrev = pCmdInput->mMouseWheelVert;
		client.mMouseWheelHorizPrev = pCmdInput->mMouseWheelHoriz;
		netImguiDeleteSafe(pCmdInput);
		return true;
	}
	return false;
}

} // namespace NetImgui

#else //NETIMGUI_ENABLED

namespace NetImgui {

bool				Startup(void)													{ return false; }
void				Shutdown(void)													{ }
bool				Connect(const char*, const char*, uint32_t)						{ return false; }
bool				Connect(StartThreadFunctPtr, const char*, const char*, uint32_t){ return false; }
void				Disconnect(void)												{ }
bool				IsConnected(void)												{ return false; }
bool				IsRemoteDraw(void)												{ return false; }
void				SendDataTexture(uint64_t, void*, uint16_t, uint16_t, eTexFormat){ }
bool				NewFrame(void)													{ return false; }
const ImDrawData*	EndFrame(void)													{ return nullptr; }
ImGuiContext*		GetRemoteContext()												{ return nullptr; }
uint8_t				GetTexture_BitsPerPixel(eTexFormat)								{ return 0; }
uint32_t			GetTexture_BytePerLine(eTexFormat, uint32_t)					{ return 0; }
uint32_t			GetTexture_BytePerImage(eTexFormat, uint32_t, uint32_t)			{ return 0; }

} // namespace NetImgui

#endif //NETIMGUI_ENABLED

#include "NetImgui_WarningDisable.h"
