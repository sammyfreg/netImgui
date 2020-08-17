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
void DefaultStartCommunicationThread(void ComFunctPtr(void*), void* pClient)
//=================================================================================================
{
	std::thread(ComFunctPtr, pClient).detach();
}

//=================================================================================================
bool ConnectToApp(const char* clientName, const char* ServerHost, uint32_t serverPort, bool bReuseLocalTime)
//=================================================================================================
{
	return ConnectToApp(DefaultStartCommunicationThread, clientName, ServerHost, serverPort, bReuseLocalTime);
}

//=================================================================================================
bool ConnectToApp(ThreadFunctPtr startThreadFunction, const char* clientName, const char* ServerHost, uint32_t serverPort, bool bReuseLocalTime)
//=================================================================================================
{
	Client::ClientInfo& client	= *gpClientInfo;	
	Disconnect();
	
	while( client.mbDisconnectRequest )
		std::this_thread::sleep_for(std::chrono::milliseconds(8));

	StringCopy(client.mName, (clientName == nullptr || clientName[0] == 0 ? "Unnamed" : clientName));
	client.mbReuseLocalTime				= bReuseLocalTime;
	client.mpSocket						= Network::Connect(ServerHost, serverPort);	
	client.mbConnectRequest				= client.mpSocket != nullptr;	
	if( client.mpSocket )
	{
		CreateImguiContext();		
		startThreadFunction(Client::CommunicationsClient, &client);		
	}
	
	return client.mpSocket != nullptr;
}

//=================================================================================================
bool ConnectFromApp(const char* clientName, uint32_t serverPort, bool bReuseLocalTime)
//=================================================================================================
{
	return ConnectFromApp(DefaultStartCommunicationThread, clientName, serverPort, bReuseLocalTime);
}

//=================================================================================================
bool ConnectFromApp(ThreadFunctPtr startThreadFunction, const char* clientName, uint32_t serverPort, bool bReuseLocalTime)
//=================================================================================================
{
	Client::ClientInfo& client = *gpClientInfo;
	Disconnect();

	while (client.mbDisconnectRequest)
		std::this_thread::sleep_for(std::chrono::milliseconds(8));

	StringCopy(client.mName, (clientName == nullptr || clientName[0] == 0 ? "Unnamed" : clientName));
	client.mbReuseLocalTime				= bReuseLocalTime;	
	client.mpSocket						= Network::ListenStart(serverPort);
	client.mbConnectRequest				= client.mpSocket != nullptr;
	if( client.mpSocket )
	{
		CreateImguiContext();
		startThreadFunction(Client::CommunicationsHost, &client);
	}
	return client.mpSocket != nullptr;
}

//=================================================================================================
void Disconnect(void)
//=================================================================================================
{
	Client::ClientInfo& client	= *gpClientInfo;
	client.mbDisconnectRequest	= client.mbConnected;
	client.mbConnectRequest		= false;
}

//=================================================================================================
bool IsConnected(void)
//=================================================================================================
{
	if( gpClientInfo )
	{
		Client::ClientInfo& client = *gpClientInfo;
		return (client.mbConnected && !client.mbDisconnectRequest) || IsRemoteDraw();
	}
	return false;
}

//=================================================================================================
bool IsConnectionPending(void)
//=================================================================================================
{
	if( gpClientInfo )
	{
		Client::ClientInfo& client = *gpClientInfo;
		return !client.mbDisconnectRequest && client.mbConnectRequest;
	}
	return false;
}

//=================================================================================================
bool IsRemoteDraw(void)
//=================================================================================================
{
	Client::ClientInfo& client	= *gpClientInfo;
	return ImGui::GetCurrentContext() == client.mpContext && client.mpContextRestore != nullptr;
}

//=================================================================================================
bool NewFrame(void)
//=================================================================================================
{
	Client::ClientInfo& client = *gpClientInfo;
	if( NetImgui::IsConnected() )
	{		
		double wantedTime				= ImGui::GetTime();
		client.mpContextRestore			= ImGui::GetCurrentContext();
		ImGui::SetCurrentContext(client.mpContext);
		if( InputUpdateData() )
		{
			float deltaTime					= static_cast<float>(wantedTime - ImGui::GetTime());
			if(!client.mbReuseLocalTime)
			{
				static auto lastTime		= std::chrono::high_resolution_clock::now();
				auto currentTime			= std::chrono::high_resolution_clock::now();
				auto duration				= std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - lastTime);
				lastTime					= currentTime;
				deltaTime					= static_cast<float>(duration.count()) / 1000000000.f;
			}
			
			ImGui::GetIO().DeltaTime		= deltaTime > 0 ? deltaTime : 1.f/1000.f;			
			ImGui::SetCurrentContext(client.mpContext);
			ImGui::NewFrame();
			return true;
		}
		ImGui::SetCurrentContext(client.mpContextRestore);
	}	
	return false;
}

//=================================================================================================
//! @note:	Be careful with the returned value, the pointer remain valid only as long as
//!			a new imgui frame hasn't been started for the netImgui remote app
const ImDrawData* EndFrame(void)
//=================================================================================================
{
	Client::ClientInfo& client		= *gpClientInfo;
	const ImDrawData* pDraw			= nullptr;
	if( ImGui::GetCurrentContext() == client.mpContext )
	{
		ImGuiMouseCursor Cursor		= ImGui::GetMouseCursor();	//Must be fetched before 'Render'
		ImGui::Render();
		pDraw						= ImGui::GetDrawData();
		CmdDrawFrame* pNewDrawFrame = CreateCmdDrawDrame(pDraw, Cursor);		
		client.mPendingFrameOut.Assign(pNewDrawFrame);
		ImGui::SetCurrentContext(client.mpContextRestore);
		client.mpContextRestore		= nullptr;

		// Client Disconnected
		if( !client.mbConnected )
		{
			ImGui::DestroyContext(client.mpContext);
			client.mpContext			= nullptr;
			pDraw						= nullptr;
		}
	}
	return pDraw;
}

//=================================================================================================
void SendDataTexture(uint64_t textureId, void* pData, uint16_t width, uint16_t height, eTexFormat format)
//=================================================================================================
{
	Client::ClientInfo& client				= *gpClientInfo;
	CmdTexture* pCmdTexture					= nullptr;
	
	// Add/Update a texture
	if( pData != nullptr )
	{		
		uint32_t PixelDataSize				= GetTexture_BytePerImage(format, width, height);
		uint32_t SizeNeeded					= PixelDataSize + sizeof(CmdTexture);
		pCmdTexture							= netImguiSizedNew<CmdTexture>(SizeNeeded);

		pCmdTexture->mpTextureData.SetPtr(reinterpret_cast<uint8_t*>(&pCmdTexture[1]));
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
		pCmdTexture							= netImguiNew<CmdTexture>();
		pCmdTexture->mTextureId				= textureId;		
		pCmdTexture->mWidth					= 0;
		pCmdTexture->mHeight				= 0;
		pCmdTexture->mFormat				= eTexFormat::kTexFmt_Invalid;
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
bool Startup(void)
//=================================================================================================
{
	gpClientInfo = netImguiNew<Client::ClientInfo>();
	return Network::Startup();
}

//=================================================================================================
void Shutdown(void)
//=================================================================================================
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
ImGuiContext* GetRemoteContext(void)
//=================================================================================================
{
	Client::ClientInfo& client = *gpClientInfo;
	return client.mpContext;
}

//=================================================================================================
uint8_t GetTexture_BitsPerPixel(eTexFormat eFormat)
//=================================================================================================
{
	switch(eFormat)
	{
	case eTexFormat::kTexFmtA8:			return 8*1;
	case eTexFormat::kTexFmtRGBA8:		return 8*4;
	case eTexFormat::kTexFmt_Invalid:	return 0;
	}
	return 0;
}

//=================================================================================================
uint32_t GetTexture_BytePerLine(eTexFormat eFormat, uint32_t pixelWidth)
//=================================================================================================
{		
	uint32_t bitsPerPixel = static_cast<uint32_t>(GetTexture_BitsPerPixel(eFormat));
	return pixelWidth * bitsPerPixel / 8;
	//Note: If adding support to BC compression format, have to take into account 4x4 size alignement
}
	
//=================================================================================================
uint32_t GetTexture_BytePerImage(eTexFormat eFormat, uint32_t pixelWidth, uint32_t pixelHeight)
//=================================================================================================
{
	return GetTexture_BytePerLine(eFormat, pixelWidth) * pixelHeight;
	//Note: If adding support to BC compression format, have to take into account 4x4 size alignement
}

//=================================================================================================
void CreateImguiContext(void)
//=================================================================================================
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

	// Import the style/options settings of current context, into this one	
	memcpy(&newStyle, &sourceStyle, sizeof(newStyle));
	memcpy(newIO.KeyMap, sourceIO.KeyMap, sizeof(newIO.KeyMap));
	newIO.ConfigFlags						= sourceIO.ConfigFlags;
	newIO.BackendFlags						= sourceIO.BackendFlags;
	//DisplaySize
	//DeltaTime
	newIO.IniSavingRate						= sourceIO.IniSavingRate;
	newIO.IniFilename						= sourceIO.IniFilename;
	newIO.LogFilename						= sourceIO.LogFilename;	
	newIO.MouseDoubleClickTime				= sourceIO.MouseDoubleClickTime;
	newIO.MouseDoubleClickMaxDist			= sourceIO.MouseDoubleClickMaxDist;
	newIO.MouseDragThreshold				= sourceIO.MouseDragThreshold;
	// KeyMap
	newIO.KeyRepeatDelay					= sourceIO.KeyRepeatDelay;
	newIO.KeyRepeatRate						= sourceIO.KeyRepeatRate;
	newIO.UserData							= sourceIO.UserData;

    newIO.FontGlobalScale					= sourceIO.FontGlobalScale;
    newIO.FontAllowUserScaling				= sourceIO.FontAllowUserScaling;
    newIO.FontDefault						= sourceIO.FontDefault; // Use same FontAtlas, so pointer is valid for new context too
    newIO.DisplayFramebufferScale			= ImVec2(1, 1);

	// Miscellaneous options
	newIO.MouseDrawCursor					= false;
	newIO.ConfigMacOSXBehaviors				= sourceIO.ConfigMacOSXBehaviors;
	newIO.ConfigInputTextCursorBlink		= sourceIO.ConfigInputTextCursorBlink;
	newIO.ConfigWindowsResizeFromEdges		= sourceIO.ConfigWindowsResizeFromEdges;
	newIO.ConfigWindowsMoveFromTitleBarOnly	= sourceIO.ConfigWindowsMoveFromTitleBarOnly;
#if IMGUI_VERSION_NUM >= 17500
	newIO.ConfigWindowsMemoryCompactTimer	= sourceIO.ConfigWindowsMemoryCompactTimer;
#endif	
	
	// Platform Functions
	newIO.BackendPlatformUserData			= sourceIO.BackendPlatformUserData;
	newIO.BackendRendererUserData			= sourceIO.BackendRendererUserData;
	newIO.BackendLanguageUserData			= sourceIO.BackendLanguageUserData;
	newIO.BackendPlatformName				= "netImgui";
	newIO.BackendRendererName				= "DirectX11";

#if defined(IMGUI_HAS_DOCK) && IMGUI_HAS_DOCK
	// Docking options (when ImGuiConfigFlags_DockingEnable is set)
	newIO.ConfigDockingNoSplit				= sourceIO.ConfigDockingNoSplit;
	newIO.ConfigDockingWithShift			= sourceIO.ConfigDockingWithShift;
	newIO.ConfigDockingAlwaysTabBar			= sourceIO.ConfigDockingAlwaysTabBar;
	newIO.ConfigDockingTransparentPayload	= sourceIO.ConfigDockingTransparentPayload;
#endif

#if defined(IMGUI_HAS_VIEWPORT) && IMGUI_HAS_VIEWPORT
	// Viewport options (when ImGuiConfigFlags_ViewportsEnable is set)
	newIO.ConfigFlags						&= ~(ImGuiConfigFlags_ViewportsEnable); // Viewport unsupported at the moment
	newIO.ConfigViewportsNoAutoMerge		= sourceIO.ConfigViewportsNoAutoMerge;
	newIO.ConfigViewportsNoTaskBarIcon		= sourceIO.ConfigViewportsNoTaskBarIcon;
	newIO.ConfigViewportsNoDecoration		= sourceIO.ConfigViewportsNoDecoration;
	newIO.ConfigViewportsNoDefaultParent	= sourceIO.ConfigViewportsNoDefaultParent;
#endif

	newIO.KeyMap[ImGuiKey_Tab]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardTab);
	newIO.KeyMap[ImGuiKey_LeftArrow]	= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardLeft);
	newIO.KeyMap[ImGuiKey_RightArrow]	= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardRight);
	newIO.KeyMap[ImGuiKey_UpArrow]		= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardUp);
	newIO.KeyMap[ImGuiKey_DownArrow]	= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardDown);
	newIO.KeyMap[ImGuiKey_PageUp]		= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardPageUp);
	newIO.KeyMap[ImGuiKey_PageDown]		= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardPageDown);
	newIO.KeyMap[ImGuiKey_Home]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardHome);
	newIO.KeyMap[ImGuiKey_End]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardEnd);
	newIO.KeyMap[ImGuiKey_Insert]		= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardInsert);
	newIO.KeyMap[ImGuiKey_Delete]		= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardDelete);
	newIO.KeyMap[ImGuiKey_Backspace]	= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardBackspace);
	newIO.KeyMap[ImGuiKey_Space]		= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardSpace);
	newIO.KeyMap[ImGuiKey_Enter]		= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardEnter);
	newIO.KeyMap[ImGuiKey_Escape]		= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardEscape);
	newIO.KeyMap[ImGuiKey_KeyPadEnter]	= 0;//static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardKeypadEnter);
	newIO.KeyMap[ImGuiKey_A]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardA);
	newIO.KeyMap[ImGuiKey_C]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardTab);
	newIO.KeyMap[ImGuiKey_V]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardTab);
	newIO.KeyMap[ImGuiKey_X]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardTab);
	newIO.KeyMap[ImGuiKey_Y]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardTab);
	newIO.KeyMap[ImGuiKey_Z]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardTab);

	ImGui::SetCurrentContext(pSourceCxt);
}

//=================================================================================================
bool InputUpdateData(void)
//=================================================================================================
{
	Client::ClientInfo& client	= *gpClientInfo;
	CmdInput* pCmdInput			= client.mPendingInputIn.Release();
	ImGuiIO& io					= ImGui::GetIO();
	if (pCmdInput)
	{
		io.DisplaySize	= ImVec2(pCmdInput->mScreenSize[0], pCmdInput->mScreenSize[1]);
		io.MousePos		= ImVec2(pCmdInput->mMousePos[0], pCmdInput->mMousePos[1]);
		io.MouseWheel	= pCmdInput->mMouseWheelVert - client.mMouseWheelVertPrev;
		io.MouseWheelH	= pCmdInput->mMouseWheelHoriz - client.mMouseWheelHorizPrev;
		io.MouseDown[0] = pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkMouseBtnLeft);
		io.MouseDown[1] = pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkMouseBtnRight);
		io.MouseDown[2] = pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkMouseBtnMid);
		io.MouseDown[3] = pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkMouseBtnExtra1);
		io.MouseDown[4] = pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkMouseBtnExtra2);
		io.KeyShift		= pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkKeyboardShift);
		io.KeyCtrl		= pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkKeyboardCtrl);
		io.KeyAlt		= pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkKeyboardAlt);
		io.KeySuper		= pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkKeyboardSuper1) || pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkKeyboardSuper2);
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
bool				Connect(ThreadFunctPtr, const char*, const char*, uint32_t)		{ return false; }
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
