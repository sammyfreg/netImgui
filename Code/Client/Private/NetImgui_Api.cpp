#include "NetImgui_Shared.h"
#include "NetImgui_WarningDisable.h"

#if NETIMGUI_ENABLED
#include <algorithm>
#include <thread>
#include "NetImgui_Client.h"
#include "NetImgui_Network.h"
#include "NetImgui_CmdPackets.h"

using namespace NetImgui::Internal;

namespace NetImgui { 

static Client::ClientInfo* gpClientInfo = nullptr;

void ContextInitialize(bool bCloneOriginalContext);
void ContextClone();
bool InputUpdateData();

//=================================================================================================
void DefaultStartCommunicationThread(void ComFunctPtr(void*), void* pClient)
//=================================================================================================
{
// Visual Studio 2017 generate this warning on std::thread, avoid the warning preventing build
#if defined(_MSC_VER) && (_MSC_VER < 1920)
	#pragma warning	(push)		
	#pragma warning (disable: 4625)		// 'std::_LaunchPad<_Target>' : copy constructor was implicitly defined as deleted
	#pragma warning (disable: 4626)		// 'std::_LaunchPad<_Target>' : assignment operator was implicitly defined as deleted
#endif

	std::thread(ComFunctPtr, pClient).detach();

#if defined(_MSC_VER) && (_MSC_VER < 1920)
	#pragma warning	(pop)
#endif	
}

//=================================================================================================
bool ConnectToApp(const char* clientName, const char* ServerHost, uint32_t serverPort, bool bCloneContext)
//=================================================================================================
{
	return ConnectToApp(DefaultStartCommunicationThread, clientName, ServerHost, serverPort, bCloneContext);
}

//=================================================================================================
bool ConnectToApp(ThreadFunctPtr startThreadFunction, const char* clientName, const char* ServerHost, uint32_t serverPort, bool bCloneContext)
//=================================================================================================
{
	if (!gpClientInfo) return false;

	Client::ClientInfo& client	= *gpClientInfo;	
	Disconnect();
	
	while (client.IsActive())
		std::this_thread::yield();

	StringCopy(client.mName, (clientName == nullptr || clientName[0] == 0 ? "Unnamed" : clientName));
	client.mpSocketPending	= Network::Connect(ServerHost, serverPort);	
	if (client.mpSocketPending)
	{
		ContextInitialize( bCloneContext );
		startThreadFunction(Client::CommunicationsClient, &client);		
	}
	
	return client.IsActive();
}

//=================================================================================================
bool ConnectFromApp(const char* clientName, uint32_t serverPort, bool bCloneContext)
//=================================================================================================
{
	return ConnectFromApp(DefaultStartCommunicationThread, clientName, serverPort, bCloneContext);
}

//=================================================================================================
bool ConnectFromApp(ThreadFunctPtr startThreadFunction, const char* clientName, uint32_t serverPort, bool bCloneContext)
//=================================================================================================
{
	if (!gpClientInfo) return false;

	Client::ClientInfo& client = *gpClientInfo;
	Disconnect();

	while (client.IsActive())
		std::this_thread::yield();

	StringCopy(client.mName, (clientName == nullptr || clientName[0] == 0 ? "Unnamed" : clientName));
	client.mpSocketPending = Network::ListenStart(serverPort);
	if (client.mpSocketPending)
	{
		ContextInitialize( bCloneContext );
		startThreadFunction(Client::CommunicationsHost, &client);
	}

	return client.IsActive();
}

//=================================================================================================
void Disconnect(void)
//=================================================================================================
{
	if (!gpClientInfo) return;
	
	Client::ClientInfo& client	= *gpClientInfo;
	client.mbDisconnectRequest	= client.IsActive();
	client.KillSocketListen(); // Forcefully disconnect Listening socket, since it is blocking
}

//=================================================================================================
bool IsConnected(void)
//=================================================================================================
{
	if (!gpClientInfo) return false;
	
	Client::ClientInfo& client = *gpClientInfo;

	// If disconnected in middle of a remote frame drawing,  
	// want to behave like it is still connected to finish frame properly
	return client.IsConnected() || IsDrawingRemote(); 
}

//=================================================================================================
bool IsConnectionPending(void)
//=================================================================================================
{
	if (!gpClientInfo) return false;
	
	Client::ClientInfo& client = *gpClientInfo;
	return client.IsConnectPending();
}

//=================================================================================================
bool IsDrawing(void)
//=================================================================================================
{
	if (!gpClientInfo) return false;

	Client::ClientInfo& client = *gpClientInfo;
	return client.mpContextDrawing == ImGui::GetCurrentContext();
}

//=================================================================================================
bool IsDrawingRemote(void)
//=================================================================================================
{
	if (!gpClientInfo) return false;

	Client::ClientInfo& client = *gpClientInfo;
	return IsDrawing() && client.mbIsRemoteDrawing;
}

//=================================================================================================
bool NewFrame(bool bSupportFrameSkip)
//=================================================================================================
{	
	if (!gpClientInfo) return false;

	Client::ClientInfo& client = *gpClientInfo;
	assert(client.mpContextDrawing == nullptr);

	// Update current active content with our time (even if it is not the one used for drawing remotely), so they are in sync
	std::chrono::duration<double> elapsedSec	= std::chrono::high_resolution_clock::now() - client.mTimeTracking;	
	ImGui::GetIO().DeltaTime					= std::max<float>(1.f / 1000.f, static_cast<float>(elapsedSec.count() - ImGui::GetTime())); 

	// ImGui Newframe handled by remote connection settings	
	if( NetImgui::IsConnected() )
	{
		client.mpContextRestore = ImGui::GetCurrentContext();
		ImGui::SetCurrentContext(client.mpContext);		
		
		// Update input and see if remote netImgui expect a new frame
		if( !InputUpdateData() )
		{
			// Caller handle skipping drawing frame, return after restoring original context
			if( bSupportFrameSkip )
			{
				ImGui::SetCurrentContext(client.mpContextRestore);
				client.mpContextRestore = nullptr;
				return false;
			} 

			// If caller doesn't handle skipping a ImGui frame rendering, assign a placeholder 
			// that will receive the ImGui draw commands and discard them
			ImGui::SetCurrentContext(client.mpContextEmpty);
		}
		// We are about to start drawing for remote context, check for font data update
		else
		{
			if( ImGui::GetIO().Fonts->TexPixelsAlpha8 && ImGui::GetIO().Fonts->TexPixelsAlpha8 != client.mpFontTextureData )
			{
				uint8_t* pPixelData(nullptr); int width(0), height(0);
				ImGui::GetIO().Fonts->GetTexDataAsAlpha8(&pPixelData, &width, &height);
				SendDataTexture( ImGui::GetIO().Fonts->TexID, pPixelData, static_cast<uint16_t>(width), static_cast<uint16_t>(height), eTexFormat::kTexFmtA8 );
			}

			// No font texture has been sent to the netImgui server, you can either 
			// 1. Leave font data available in ImGui (not call ImGui::ClearTexData) for netImgui to auto send it
			// 2. Manually call 'NetImgui::SendDataTexture' with font texture data
			assert(client.mbFontUploaded);
		}
	}
	// Regular Imgui NewFrame
	else
	{		
		client.mSavedContextValues.Restore(ImGui::GetCurrentContext());	// Restore context setting override, after a disconnect (if applicable)
	}

	// A new frame is expected, update the current time of the drawing context, and let Imgui know to prepare a new drawing frame	
	client.mbIsRemoteDrawing	= NetImgui::IsConnected();
	client.mpContextDrawing		= ImGui::GetCurrentContext();
	ImGui::GetIO().DeltaTime	= std::max<float>(1.f / 1000.f, static_cast<float>(elapsedSec.count() - ImGui::GetTime()));
	ImGui::NewFrame();
	return true;
}

//=================================================================================================
void EndFrame(void)
//=================================================================================================
{
	if (!gpClientInfo) return;

	Client::ClientInfo& client		= *gpClientInfo;	

	if (client.mpContextDrawing)
	{
		const bool bDiscardDraw		= client.mpContextDrawing == client.mpContextEmpty;
		client.mpContextRestore		= client.mpContextRestore ? client.mpContextRestore : ImGui::GetCurrentContext();
		ImGui::SetCurrentContext(client.mpContextDrawing);		// In case it was changed and not restored after BeginFrame call
		ImGuiMouseCursor Cursor		= ImGui::GetMouseCursor();	// Must be fetched before 'Render'
		ImGui::Render();
		
		// We were drawing frame for our remote connection, send the data
		if (IsConnected() && !bDiscardDraw)
		{
			CmdDrawFrame* pNewDrawFrame = CreateCmdDrawDrame(ImGui::GetDrawData(), Cursor);
			client.mPendingFrameOut.Assign(pNewDrawFrame);
		}
				
		if (ImGui::GetCurrentContext() == client.mpContextClone)
			ImGui::GetIO().DeltaTime= 0.f; // Reset the time passed. Gets incremented in NewFrame

		ImGui::SetCurrentContext(client.mpContextRestore);		
	}
	client.mpContextRestore			= nullptr;
	client.mpContextDrawing			= nullptr;
	client.mbIsRemoteDrawing		= false;
}

//=================================================================================================
ImGuiContext* GetDrawingContext()
//=================================================================================================
{
	if (!gpClientInfo) return nullptr;

	Client::ClientInfo& client = *gpClientInfo;
	return client.mpContextDrawing;
}


//=================================================================================================
ImDrawData* GetDrawData(void)
//=================================================================================================
{
	if (!gpClientInfo) return nullptr;

	Client::ClientInfo& client	= *gpClientInfo;	
	ImDrawData* pLastFrameDrawn = ImGui::GetDrawData();
	if (IsConnected() && client.mpContextClone)
	{
		ScopedImguiContext scopedCtx(client.mpContextClone);
		pLastFrameDrawn			= ImGui::GetDrawData();
	}
	return pLastFrameDrawn;
}

//=================================================================================================
void SendDataTexture(ImTextureID textureId, void* pData, uint16_t width, uint16_t height, eTexFormat format)
//=================================================================================================
{
	if (!gpClientInfo) return;

	Client::ClientInfo& client				= *gpClientInfo;
	CmdTexture* pCmdTexture					= nullptr;

	// Makes sure even 32bits ImTextureID value are received properly as 64bits
	uint64_t texId64(0);
	static_assert(sizeof(uint64_t) <= sizeof(textureId), "ImTextureID is bigger than 64bits, CmdTexture::mTextureId needs to be updated to support it");
	reinterpret_cast<ImTextureID*>(&texId64)[0] = textureId;

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
		pCmdTexture->mTextureId				= texId64;
		pCmdTexture->mFormat				= format;
		pCmdTexture->mpTextureData.ToOffset();

		// Detects when user is sending the font texture
		ScopedImguiContext scopedCtx(client.mpContext ? client.mpContext : ImGui::GetCurrentContext());
		if( ImGui::GetIO().Fonts && ImGui::GetIO().Fonts->TexID == textureId )
		{
			client.mbFontUploaded		|= true;
			client.mpFontTextureData	= ImGui::GetIO().Fonts->TexPixelsAlpha8;
		}		
	}
	// Texture to remove
	else
	{
		pCmdTexture							= netImguiNew<CmdTexture>();
		pCmdTexture->mTextureId				= texId64;		
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
			std::this_thread::yield();
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
	if( !gpClientInfo )
	{
		gpClientInfo = netImguiNew<Client::ClientInfo>();	
	}
	gpClientInfo->mTimeTracking	= std::chrono::high_resolution_clock::now();
	return Network::Startup();
}

//=================================================================================================
void Shutdown(bool bWait)
//=================================================================================================
{
	if (!gpClientInfo) return;
	
	Disconnect();
	while(bWait && gpClientInfo->IsActive() )
		std::this_thread::yield();
	Network::Shutdown();
	
	for( auto& texture : gpClientInfo->mTextures )
		texture.Set(nullptr);

	if( gpClientInfo->mpContextClone )
		ImGui::DestroyContext(gpClientInfo->mpContextClone);

	if( gpClientInfo->mpContextEmpty )
		ImGui::DestroyContext(gpClientInfo->mpContextEmpty);

	netImguiDeleteSafe(gpClientInfo);		
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
	//Note: If adding support to BC compression format, have to take into account 4x4 size alignment
}
	
//=================================================================================================
uint32_t GetTexture_BytePerImage(eTexFormat eFormat, uint32_t pixelWidth, uint32_t pixelHeight)
//=================================================================================================
{
	return GetTexture_BytePerLine(eFormat, pixelWidth) * pixelHeight;
	//Note: If adding support to BC compression format, have to take into account 4x4 size alignement
}

// Support of the callback hooks
#if IMGUI_HAS_CALLBACK
void HookBeginFrame(ImGuiContext* ctx, ImGuiContextHook* hook)
{
	Client::ClientInfo& client = *reinterpret_cast<Client::ClientInfo*>(hook->UserData);
	(void)ctx;
	(void)client;
}

void HookEndFrame(ImGuiContext* ctx, ImGuiContextHook* hook)
{
	Client::ClientInfo& client = *reinterpret_cast<Client::ClientInfo*>(hook->UserData);
	(void)ctx;
	(void)client;
}
#endif 	// IMGUI_HAS_CALLBACK

//=================================================================================================
void ContextInitialize(bool bCloneOriginalContext)
//=================================================================================================
{
	if (!gpClientInfo) return;

	Client::ClientInfo& client = *gpClientInfo;
	if (client.mpContextClone)
	{
		ImGui::DestroyContext(client.mpContextClone);
		client.mpContextClone = nullptr;
	}

	// Create a placeholder Context. Used to output ImGui drawing commands to, 
	// when User expect to be able to draw even though the remote connection 
	// doesn't expect any new drawing for this frame
	if (client.mpContextEmpty == nullptr)
	{
		client.mpContextEmpty = ImGui::CreateContext(ImGui::GetIO().Fonts);
		ScopedImguiContext scopedCtx(client.mpContextEmpty);
		ImGui::GetIO().DeltaTime = 1.f / 30.f;
		ImGui::GetIO().DisplaySize = ImVec2(1, 1);
	}
	
	if( bCloneOriginalContext )
	{
		ContextClone();
		client.mpContext = client.mpContextClone;
	}
	else
	{	
		client.mpContext = ImGui::GetCurrentContext();
		client.mSavedContextValues.Save(client.mpContext); 
	}

	// Override some settings
	// Note: Make sure every setting overwritten here, are handled in 'SavedImguiContext::Save(...)'
	{
		ScopedImguiContext scopedCtx(client.mpContext);
		ImGuiIO& newIO						= ImGui::GetIO();	
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
	#if IMGUI_VERSION_NUM >= 17102
		newIO.KeyMap[ImGuiKey_KeyPadEnter]	= 0;//static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardKeypadEnter);
	#endif
		newIO.KeyMap[ImGuiKey_A]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardA);
		newIO.KeyMap[ImGuiKey_C]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardA) - 'A' + 'C';
		newIO.KeyMap[ImGuiKey_V]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardA) - 'A' + 'V';
		newIO.KeyMap[ImGuiKey_X]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardA) - 'A' + 'X';
		newIO.KeyMap[ImGuiKey_Y]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardA) - 'A' + 'Y';
		newIO.KeyMap[ImGuiKey_Z]			= static_cast<int>(CmdInput::eVirtualKeys::vkKeyboardA) - 'A' + 'Z';

		newIO.ClipboardUserData				= nullptr;
	
		newIO.BackendPlatformName			= "NetImgui";
		newIO.BackendRendererName			= "DirectX11";
	#if IMGUI_VERSION_NUM >= 17700
		newIO.ImeWindowHandle				= nullptr;
	#endif
	#if defined(IMGUI_HAS_VIEWPORT) && IMGUI_HAS_VIEWPORT
		// Viewport options (when ImGuiConfigFlags_ViewportsEnable is set)
		newIO.ConfigFlags					&= ~(ImGuiConfigFlags_ViewportsEnable); // Viewport unsupported at the moment
	#endif
	}

	// Support of the callback hooks
#if IMGUI_HAS_CALLBACK
	client.mImguiHookNewframe.Type		= ImGuiContextHookType_NewFramePre;
	client.mImguiHookNewframe.Callback	= HookBeginFrame;
	client.mImguiHookNewframe.UserData	= &client;			//SF TODO: Save previous hook, to chain them
	client.mImguiHookEndframe.Type		= ImGuiContextHookType_EndFramePost;
	client.mImguiHookEndframe.Callback	= HookEndFrame;
	client.mImguiHookEndframe.UserData	= &client;			//SF TODO: Save previous hook, to chain them
	ImGui::AddContextHook(ImGui::GetCurrentContext(), &client.mImguiHookNewframe);
	ImGui::AddContextHook(ImGui::GetCurrentContext(), &client.mImguiHookEndframe);
#endif
}

//=================================================================================================
void ContextClone(void)
//=================================================================================================
{
	if (!gpClientInfo) return;

	Client::ClientInfo& client				= *gpClientInfo;	
	client.mpContextClone					= ImGui::CreateContext(ImGui::GetIO().Fonts);
	ImGuiIO& sourceIO						= ImGui::GetIO();
	ImGuiStyle& sourceStyle					= ImGui::GetStyle();
	{
		ScopedImguiContext scopedCtx(client.mpContextClone);
		ImGuiIO& newIO							= ImGui::GetIO();
		ImGuiStyle& newStyle					= ImGui::GetStyle();

		// Import the style/options settings of current context, into this one	
		memcpy(&newStyle, &sourceStyle, sizeof(newStyle));
		//memcpy(newIO.KeyMap, sourceIO.KeyMap, sizeof(newIO.KeyMap));
		memcpy(&newIO, &sourceIO, sizeof(newIO));		
		newIO.InputQueueCharacters.Data		= 0;
		newIO.InputQueueCharacters.Size		= 0;
		newIO.InputQueueCharacters.Capacity = 0;
	}
}

//=================================================================================================
bool InputUpdateData(void)
//=================================================================================================
{
	if (!gpClientInfo) return false;

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
		//io.NavInputs // @Sammyfreg TODO: Handle Gamepad

		memset(io.KeysDown, 0, sizeof(io.KeysDown));
		for (uint32_t i(0); i < ArrayCount(pCmdInput->mKeysDownMask) * 64; ++i)
			io.KeysDown[i] = (pCmdInput->mKeysDownMask[i / 64] & (uint64_t(1) << (i % 64))) != 0;

		// @Sammyfreg TODO: Optimize this
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

#ifdef IMGUI_VERSION
static bool 		sIsDrawing = false;
#endif

bool				Startup(void)															{ return true; }
void				Shutdown(bool)															{ }
bool				ConnectToApp(const char*, const char*, uint32_t, bool)					{ return false; }
bool				ConnectToApp(ThreadFunctPtr, const char*, const char*, uint32_t, bool)	{ return false; }
bool				ConnectFromApp(const char*, uint32_t, bool)								{ return false; }
bool				ConnectFromApp(ThreadFunctPtr, const char*, uint32_t, bool)				{ return false; }
void				Disconnect(void)														{ }
bool				IsConnected(void)														{ return false; }
bool				IsDrawingRemote(void)													{ return false; }
bool				IsConnectionPending(void)												{ return false; }
void				SendDataTexture(uint64_t, void*, uint16_t, uint16_t, eTexFormat)		{ }
uint8_t				GetTexture_BitsPerPixel(eTexFormat)										{ return 0; }
uint32_t			GetTexture_BytePerLine(eTexFormat, uint32_t)							{ return 0; }
uint32_t			GetTexture_BytePerImage(eTexFormat, uint32_t, uint32_t)					{ return 0; }

//=================================================================================================
// If ImGui is enabled but not NetImgui, handle the BeginFrame/EndFrame normally
//=================================================================================================
bool NewFrame(bool)													
{ 
#ifdef IMGUI_VERSION
	if( !sIsDrawing )
	{
		sIsDrawing = true;
		ImGui::NewFrame();
		return true;
	}
#endif
	return false; 

}
void EndFrame(void)													
{
#ifdef IMGUI_VERSION
	if( sIsDrawing )
	{		
		ImGui::Render();
		sIsDrawing = false;
	}
#endif
}

ImDrawData* GetDrawData(void)												
{ 
#ifdef IMGUI_VERSION	
	return ImGui::GetDrawData();
#else
	return nullptr;
#endif
}

bool IsDrawing(void)
{ 
#ifdef IMGUI_VERSION
	return sIsDrawing; 
#else
	return false;
#endif
}

} // namespace NetImgui

#endif //NETIMGUI_ENABLED

#include "NetImgui_WarningDisable.h"
