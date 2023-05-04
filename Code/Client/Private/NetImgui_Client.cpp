#include "NetImgui_Shared.h"

#if NETIMGUI_ENABLED
#include "NetImgui_WarningDisable.h"
#include "NetImgui_Client.h"
#include "NetImgui_Network.h"
#include "NetImgui_CmdPackets.h"

namespace NetImgui { namespace Internal { namespace Client 
{

//=================================================================================================
// SAVED IMGUI CONTEXT
// Because we overwrite some Imgui context IO values, we save them before makign any change
// and restore them after detecting a disconnection
//=================================================================================================
void SavedImguiContext::Save(ImGuiContext* copyFrom)
{
	ScopedImguiContext scopedContext(copyFrom);
	ImGuiIO& sourceIO		= ImGui::GetIO();
	mSavedContext			= true;
	mConfigFlags			= sourceIO.ConfigFlags;
	mBackendFlags			= sourceIO.BackendFlags;
	mBackendPlatformName	= sourceIO.BackendPlatformName;
	mBackendRendererName	= sourceIO.BackendRendererName;
	mDrawMouse				= sourceIO.MouseDrawCursor;
	mClipboardUserData		= sourceIO.ClipboardUserData;
#if IMGUI_VERSION_NUM < 18700
	memcpy(mKeyMap, sourceIO.KeyMap, sizeof(mKeyMap));
#endif
}

void SavedImguiContext::Restore(ImGuiContext* copyTo)
{	
	ScopedImguiContext scopedContext(copyTo);
	ImGuiIO& destIO				= ImGui::GetIO();
	mSavedContext				= false;
	destIO.ConfigFlags			= mConfigFlags;
	destIO.BackendFlags			= mBackendFlags;
	destIO.BackendPlatformName	= mBackendPlatformName;
	destIO.BackendRendererName	= mBackendRendererName;
	destIO.MouseDrawCursor		= mDrawMouse;
	destIO.ClipboardUserData	= mClipboardUserData;
#if IMGUI_VERSION_NUM < 18700
	memcpy(destIO.KeyMap, mKeyMap, sizeof(destIO.KeyMap));
#endif
}

//=================================================================================================
// COMMUNICATIONS INITIALIZE
// Initialize a new connection to a RemoteImgui server
//=================================================================================================
bool Communications_Initialize(ClientInfo& client)
{
	CmdVersion cmdVersionSend, cmdVersionRcv;
	StringCopy(cmdVersionSend.mClientName, client.mName);
	bool bResultSend	= Network::DataSend(client.mpSocketPending, &cmdVersionSend, cmdVersionSend.mHeader.mSize);
	bool bResultRcv		= Network::DataReceive(client.mpSocketPending, &cmdVersionRcv, sizeof(cmdVersionRcv));
	bool mbConnected	= bResultRcv && bResultSend && 
						  cmdVersionRcv.mHeader.mType	== cmdVersionSend.mHeader.mType && 
						  cmdVersionRcv.mVersion		== cmdVersionSend.mVersion &&
						  cmdVersionRcv.mWCharSize		== cmdVersionSend.mWCharSize;	
	if(mbConnected)
	{				
		for(auto& texture : client.mTextures)
		{
			texture.mbSent = false;
		}

		client.mbHasTextureUpdate			= true;								// Force sending the client textures
		client.mBGSettingSent.mTextureId	= client.mBGSetting.mTextureId-1u;	// Force sending the Background settings (by making different than current settings)
		client.mpSocketComs					= client.mpSocketPending.exchange(nullptr);
		client.mFrameIndex					= 0;
	}
	return client.mpSocketComs.load() != nullptr;
}

//=================================================================================================
// INCOM: INPUT
// Receive new keyboard/mouse/screen resolution input to pass on to dearImgui
//=================================================================================================
void Communications_Incoming_Input(ClientInfo& client, uint8_t*& pCmdData)
{
	if( pCmdData )
	{
		auto pCmdInput	= reinterpret_cast<CmdInput*>(pCmdData);
		pCmdData		= nullptr; // Take ownership of the data, prevent Free
		size_t keyCount(pCmdInput->mKeyCharCount);
		client.mPendingKeyIn.AddData(pCmdInput->mKeyChars, keyCount);
		client.mPendingInputIn.Assign(pCmdInput);	
	}
}

//=================================================================================================
// OUTCOM: TEXTURE
// Transmit all pending new/updated texture
//=================================================================================================
bool Communications_Outgoing_Textures(ClientInfo& client)
{	
	bool bSuccess(true);
	client.ProcessTexturePending();
	if( client.mbHasTextureUpdate )
	{
		for(auto& cmdTexture : client.mTextures)
		{
			if( !cmdTexture.mbSent && cmdTexture.mpCmdTexture )
			{
				bSuccess			&= Network::DataSend(client.mpSocketComs, cmdTexture.mpCmdTexture, cmdTexture.mpCmdTexture->mHeader.mSize);
				cmdTexture.mbSent	= bSuccess;
				if( cmdTexture.mbSent && cmdTexture.mpCmdTexture->mFormat == eTexFormat::kTexFmt_Invalid )
					netImguiDeleteSafe(cmdTexture.mpCmdTexture);					
			}
		}
		client.mbHasTextureUpdate = !bSuccess;
	}
	return bSuccess;
}

//=================================================================================================
// OUTCOM: BACKGROUND
// Transmit the current client background settings
//=================================================================================================
bool Communications_Outgoing_Background(ClientInfo& client)
{	
	bool bSuccess(true);
	CmdBackground* pPendingBackground = client.mPendingBackgroundOut.Release();
	if( pPendingBackground )
	{
		bSuccess = Network::DataSend(client.mpSocketComs, pPendingBackground, pPendingBackground->mHeader.mSize);
		netImguiDeleteSafe(pPendingBackground);
	}
	return bSuccess;
}

//=================================================================================================
// OUTCOM: FRAME
// Transmit a new dearImgui frame to render
//=================================================================================================
bool Communications_Outgoing_Frame(ClientInfo& client)
{
	bool bSuccess(true);
	CmdDrawFrame* pPendingDrawFrame = client.mPendingFrameOut.Release();
	if( pPendingDrawFrame )
	{
		pPendingDrawFrame->mFrameIndex	= client.mFrameIndex++;
		//---------------------------------------------------------------------
		// Apply delta compression to DrawCommand, when requested
		if( pPendingDrawFrame->mCompressed )
		{
			// Create a new Compressed DrawFrame Command
			if( client.mpDrawFramePrevious && !client.mServerCompressionSkip ){
				client.mpDrawFramePrevious->ToPointers();
				CmdDrawFrame* pDrawFrameCompressed	= CompressCmdDrawFrame(client.mpDrawFramePrevious, pPendingDrawFrame);
				netImguiDeleteSafe(client.mpDrawFramePrevious);
				client.mpDrawFramePrevious			= pPendingDrawFrame;	// Keep original new command for next frame delta compression
				pPendingDrawFrame					= pDrawFrameCompressed;	// Request compressed copy to be sent to server
			}
			// Save DrawCmd for next frame delta compression
			else {
				pPendingDrawFrame->mCompressed		= false;
				client.mpDrawFramePrevious			= pPendingDrawFrame;
			}
		}
		client.mServerCompressionSkip = false;

		//---------------------------------------------------------------------
		// Send Command to server
		pPendingDrawFrame->ToOffsets();
		bSuccess = Network::DataSend(client.mpSocketComs, pPendingDrawFrame, pPendingDrawFrame->mHeader.mSize);

		//---------------------------------------------------------------------
		// Free created data once sent (when not used in next frame)
		if( client.mpDrawFramePrevious != pPendingDrawFrame ){
			netImguiDeleteSafe(pPendingDrawFrame);
		}
	}
	return bSuccess;
}

//=================================================================================================
// OUTCOM: DISCONNECT
// Signal that we will be disconnecting
//=================================================================================================
bool Communications_Outgoing_Disconnect(ClientInfo& client)
{
	if( client.mbDisconnectRequest )
	{
		CmdDisconnect cmdDisconnect;
		Network::DataSend(client.mpSocketComs, &cmdDisconnect, cmdDisconnect.mHeader.mSize);
		return false;
	}
	return true;
}

//=================================================================================================
// OUTCOM: PING
// Signal end of outgoing communications and still alive
//=================================================================================================
bool Communications_Outgoing_Ping(ClientInfo& client)
{
	CmdPing cmdPing;
	return Network::DataSend(client.mpSocketComs, &cmdPing, cmdPing.mHeader.mSize);		
}

//=================================================================================================
// INCOMING COMMUNICATIONS
//=================================================================================================
bool Communications_Incoming(ClientInfo& client)
{
	bool bOk(true);
	bool bPingReceived(false);
	while( bOk && !bPingReceived )
	{
		CmdHeader cmdHeader;
		uint8_t* pCmdData	= nullptr;
		bOk					= Network::DataReceive(client.mpSocketComs, &cmdHeader, sizeof(cmdHeader));
		if( bOk && cmdHeader.mSize > sizeof(CmdHeader) )
		{
			pCmdData								= netImguiSizedNew<uint8_t>(cmdHeader.mSize);
			*reinterpret_cast<CmdHeader*>(pCmdData) = cmdHeader;
			bOk										= Network::DataReceive(client.mpSocketComs, &pCmdData[sizeof(cmdHeader)], cmdHeader.mSize-sizeof(cmdHeader));	
		}

		if( bOk )
		{
			switch( cmdHeader.mType )
			{
			case CmdHeader::eCommands::Ping:		bPingReceived = true; break;
			case CmdHeader::eCommands::Disconnect:	bOk = false; break;
			case CmdHeader::eCommands::Input:		Communications_Incoming_Input(client, pCmdData); break;
			// Commands not received in main loop, by Client
			case CmdHeader::eCommands::Invalid:
			case CmdHeader::eCommands::Version:
			case CmdHeader::eCommands::Texture:
			case CmdHeader::eCommands::DrawFrame:	
			case CmdHeader::eCommands::Background:	break;
			}
		}		
		netImguiDeleteSafe(pCmdData);
	}
	return bOk;
}

//=================================================================================================
// OUTGOING COMMUNICATIONS
//=================================================================================================
bool Communications_Outgoing(ClientInfo& client)
{		
	bool bSuccess(true);
	if( bSuccess )
		bSuccess = Communications_Outgoing_Textures(client);
	if( bSuccess )
		bSuccess = Communications_Outgoing_Background(client);
	if( bSuccess )
		bSuccess = Communications_Outgoing_Frame(client);
	if( bSuccess )
		bSuccess = Communications_Outgoing_Disconnect(client);
	if( bSuccess )
		bSuccess = Communications_Outgoing_Ping(client); // Always finish with a ping

	return bSuccess;
}

//=================================================================================================
// COMMUNICATIONS THREAD 
//=================================================================================================
void CommunicationsClient(void* pClientVoid)
{	
	ClientInfo* pClient				= reinterpret_cast<ClientInfo*>(pClientVoid);
	pClient->mbClientThreadActive	= true;
	pClient->mbDisconnectRequest	= false;
	Communications_Initialize(*pClient);
	bool bConnected					= pClient->IsConnected();
	
	while( bConnected && !pClient->mbDisconnectRequest )
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		//std::this_thread::yield();
		bConnected = Communications_Outgoing(*pClient) && Communications_Incoming(*pClient);
	}

	pClient->KillSocketComs();
	pClient->mbClientThreadActive	= false;
}

//=================================================================================================
// COMMUNICATIONS WAIT THREAD 
//=================================================================================================
void CommunicationsHost(void* pClientVoid)
{
	ClientInfo* pClient				= reinterpret_cast<ClientInfo*>(pClientVoid);
	pClient->mbListenThreadActive	= true;
	pClient->mbDisconnectRequest	= false;
	pClient->mpSocketListen			= pClient->mpSocketPending.exchange(nullptr);
	
	while( pClient->mpSocketListen.load() != nullptr && !pClient->mbDisconnectRequest )
	{
		pClient->mpSocketPending = Network::ListenConnect(pClient->mpSocketListen);
		if( pClient->mpSocketPending.load() != nullptr )
		{
			bool bConnected = Communications_Initialize(*pClient);
			while (bConnected && !pClient->mbDisconnectRequest)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				//std::this_thread::yield();
				bConnected	= Communications_Outgoing(*pClient) && Communications_Incoming(*pClient);
			}
			pClient->KillSocketComs();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));	// Prevents this thread from taking entire core, waiting on server connection
	}
	pClient->KillSocketListen();
	pClient->mbListenThreadActive	= false;
}

//=================================================================================================
// Support of the Dear ImGui hooks 
// (automatic call to NetImgui::BeginFrame()/EndFrame() on ImGui::BeginFrame()/Imgui::Render()
//=================================================================================================
#if NETIMGUI_IMGUI_CALLBACK_ENABLED
void HookBeginFrame(ImGuiContext*, ImGuiContextHook* hook)
{
	Client::ClientInfo& client = *reinterpret_cast<Client::ClientInfo*>(hook->UserData);
	if (!client.mbInsideNewEnd)
	{
		ScopedBool scopedInside(client.mbInsideHook, true);
		NetImgui::NewFrame(false);
	}
}

void HookEndFrame(ImGuiContext*, ImGuiContextHook* hook)
{
	Client::ClientInfo& client = *reinterpret_cast<Client::ClientInfo*>(hook->UserData);
	if (!client.mbInsideNewEnd)
	{
		ScopedBool scopedInside(client.mbInsideHook, true);
		NetImgui::EndFrame();
	}
}

#endif 	// NETIMGUI_IMGUI_CALLBACK_ENABLED

//=================================================================================================
// CLIENT INFO Constructor
//=================================================================================================
ClientInfo::ClientInfo()
: mpSocketPending(nullptr)
, mpSocketComs(nullptr)
, mpSocketListen(nullptr)
, mFontTextureID(TextureCastFromUInt(uint64_t(0u)))
, mTexturesPendingSent(0)
, mTexturesPendingCreated(0)
{
	memset(mTexturesPending, 0, sizeof(mTexturesPending));
}

//=================================================================================================
// CLIENT INFO Constructor
//=================================================================================================
ClientInfo::~ClientInfo()
{
	ContextRemoveHooks();

	for( auto& texture : mTextures ){
		texture.Set(nullptr);
	}

	for(size_t i(0); i<ArrayCount(mTexturesPending); ++i){
		netImguiDeleteSafe(mTexturesPending[i]);
	}

	netImguiDeleteSafe(mpInputPending);
	netImguiDeleteSafe(mpDrawFramePrevious);
}

//=================================================================================================
// Initialize the associated ImguiContext
//=================================================================================================
void ClientInfo::ContextInitialize()
{
	mpContext				= ImGui::GetCurrentContext();

#if NETIMGUI_IMGUI_CALLBACK_ENABLED
	ImGuiContextHook hookNewframe, hookEndframe;
	ContextRemoveHooks();
	hookNewframe.HookId		= 0;
	hookNewframe.Type		= ImGuiContextHookType_NewFramePre;
	hookNewframe.Callback	= HookBeginFrame;
	hookNewframe.UserData	= this;	
	mhImguiHookNewframe		= ImGui::AddContextHook(mpContext, &hookNewframe);
	hookEndframe.HookId		= 0;
	hookEndframe.Type		= ImGuiContextHookType_RenderPost;
	hookEndframe.Callback	= HookEndFrame;
	hookEndframe.UserData	= this;
	mhImguiHookEndframe		= ImGui::AddContextHook(mpContext, &hookEndframe);
#endif
}

//=================================================================================================
// Take over a Dear ImGui context for use with NetImgui
//=================================================================================================
void ClientInfo::ContextOverride()
{
	ScopedImguiContext scopedSourceCtx(mpContext);

	// Keep a copy of original settings of this context	
	mSavedContextValues.Save(mpContext);
	mTimeTracking = std::chrono::high_resolution_clock::now();

	// Override some settings
	// Note: Make sure every setting overwritten here, are handled in 'SavedImguiContext::Save(...)'
	{
		ImGuiIO& newIO						= ImGui::GetIO();
		newIO.MouseDrawCursor				= false;
		newIO.ClipboardUserData				= nullptr;
		newIO.BackendPlatformName			= "NetImgui";
		newIO.BackendRendererName			= "DirectX11";
#if IMGUI_VERSION_NUM < 18700
		for (uint32_t i(0); i < ImGuiKey_COUNT; ++i) {
			newIO.KeyMap[i] = i;
		}
#endif
#if defined(IMGUI_HAS_VIEWPORT)
		newIO.ConfigFlags					&= ~(ImGuiConfigFlags_ViewportsEnable); // Viewport unsupported at the moment
#endif
	}
}

//=================================================================================================
// Restore a Dear ImGui context to initial state before we modified it
//=================================================================================================
void ClientInfo::ContextRestore()
{
	// Note: only happens if context overriden is same as current one, to prevent trying to restore to a deleted context
	if (IsContextOverriden() && ImGui::GetCurrentContext() == mpContext)
	{
#ifdef IMGUI_HAS_VIEWPORT
		ImGui::UpdatePlatformWindows(); // Prevents issue with mismatched frame tracking, when restoring enabled viewport feature
#endif
		mSavedContextValues.Restore(mpContext);
	}
}

//=================================================================================================
// Remove callback hooks, once we detect a disconnection
//=================================================================================================
void ClientInfo::ContextRemoveHooks()
{
#if NETIMGUI_IMGUI_CALLBACK_ENABLED
	if (mpContext && mhImguiHookNewframe != 0)
	{
		ImGui::RemoveContextHook(mpContext, mhImguiHookNewframe);
		ImGui::RemoveContextHook(mpContext, mhImguiHookEndframe);
		mhImguiHookNewframe = mhImguiHookNewframe = 0;
	}
#endif
}

//=================================================================================================
// Process textures waiting to be sent to server
// 1. New textures are added tp pending queue (Main Thread)
// 2. Pending textures are sent to Server and added to our active texture list (Com Thread) 
//=================================================================================================
void ClientInfo::ProcessTexturePending()
{
	while( mTexturesPendingCreated != mTexturesPendingSent )
	{
		mbHasTextureUpdate			|= true;
		uint32_t idx				= mTexturesPendingSent.fetch_add(1) % static_cast<uint32_t>(ArrayCount(mTexturesPending));
		CmdTexture* pCmdTexture		= mTexturesPending[idx];
		mTexturesPending[idx]		= nullptr;
		if( pCmdTexture )
		{
			// Find the TextureId from our list (or free slot)
			int texIdx		= 0;
			int texFreeSlot	= static_cast<int>(mTextures.size());
			while( texIdx < mTextures.size() && ( !mTextures[texIdx].IsValid() || mTextures[texIdx].mpCmdTexture->mTextureId != pCmdTexture->mTextureId) )
			{
				texFreeSlot = !mTextures[texIdx].IsValid() ? texIdx : texFreeSlot;
				++texIdx;
			}

			if( texIdx == mTextures.size() )
				texIdx = texFreeSlot;
			if( texIdx == mTextures.size() )
				mTextures.push_back(ClientTexture());

			mTextures[texIdx].Set( pCmdTexture );
			mTextures[texIdx].mbSent = false;
		}
	}
}

//=================================================================================================
// Create a new Draw Command from Dear Imgui draw data. 
// 1. New ImGui frame has been completed, create a new draw command from draw data (Main Thread)
// 2. We see a pending Draw Command, take ownership of it and send it to Server (Com thread)
//=================================================================================================
void ClientInfo::ProcessDrawData(const ImDrawData* pDearImguiData, ImGuiMouseCursor mouseCursor)
{
	if( !mbValidDrawFrame )
		return;

	CmdDrawFrame* pDrawFrameNew = ConvertToCmdDrawFrame(pDearImguiData, mouseCursor);
	pDrawFrameNew->mCompressed	= mClientCompressionMode == eCompressionMode::kForceEnable || (mClientCompressionMode == eCompressionMode::kUseServerSetting && mServerCompressionEnabled);
	mPendingFrameOut.Assign(pDrawFrameNew);
}

}}} // namespace NetImgui::Internal::Client

#include "NetImgui_WarningReenable.h"
#endif //#if NETIMGUI_ENABLED
