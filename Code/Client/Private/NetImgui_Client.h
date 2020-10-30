#pragma once

#include "NetImgui_Shared.h"

//=============================================================================
// Forward Declares
//=============================================================================
namespace NetImgui { namespace Internal { struct CmdTexture; struct CmdDrawFrame; struct CmdInput; } }
namespace NetImgui { namespace Internal { namespace Network { struct SocketInfo; } } }

namespace NetImgui { namespace Internal { namespace Client
{

//=============================================================================
// Keep a list of textures used by Imgui, needed by server
//=============================================================================
struct ClientTexture
{
	inline void	Set( CmdTexture* pCmdTexture );
	inline bool	IsValid()const;	
	CmdTexture* mpCmdTexture= nullptr;
	bool		mbSent		= false;
	uint8_t		mPadding[7]	= {0};
};

//=============================================================================
// Keeps a list of ImGui context values NetImgui overrides (to restore)
//=============================================================================
struct SavedImguiContext
{
	void					Save(ImGuiContext* copyFrom);
	void					Restore(ImGuiContext* copyTo);	
	const char*				mBackendPlatformName	= nullptr;
	const char*				mBackendRendererName	= nullptr;	
	void*					mClipboardUserData		= nullptr;
    void*					mImeWindowHandle		= nullptr;
	ImGuiContext*			mpCopiedContext			= nullptr;	
	ImGuiBackendFlags		mBackendFlags			= 0;
	ImGuiConfigFlags		mConfigFlags			= 0;	
	bool					mDrawMouse				= false;
	char					mPadding1[7];
	int						mKeyMap[ImGuiKey_COUNT];
	char					mPadding2[8 - (sizeof(mKeyMap) % 8)];	
};

//=============================================================================
// Keep all Client infos needed for communication with server
//=============================================================================
struct ClientInfo
{
	using VecTexture	= ImVector<ClientTexture>;
	using BufferKeys	= Ringbuffer<uint16_t, 1024>;
	using Time			= std::chrono::time_point<std::chrono::high_resolution_clock>;

										ClientInfo();
	std::atomic<Network::SocketInfo*>	mpSocketPending;						// Hold socket info until communication is established
	std::atomic<Network::SocketInfo*>	mpSocketComs;							// Socket used for communications with server
	std::atomic<Network::SocketInfo*>	mpSocketListen;							// Socket used to wait for communication request from server
	char								mName[16]					={0};
	VecTexture							mTextures;
	CmdTexture*							mTexturesPending[16];
	ExchangePtr<CmdDrawFrame>			mPendingFrameOut;
	ExchangePtr<CmdInput>				mPendingInputIn;
	BufferKeys							mPendingKeyIn;
	ImGuiContext*						mpContextClone				= nullptr;	// Default ImGui drawing context copy, used to do our internal remote drawing (client obj has ownership)
	ImGuiContext*						mpContextEmpty				= nullptr;	// Placeholder ImGui drawing context, when we are not waiting for a new drawing frame but still want a valid context in place (client obj has ownership)
	ImGuiContext*						mpContextRestore			= nullptr;	// Context to restore to Imgui once drawing is done
	ImGuiContext*						mpContextDrawing			= nullptr;	// Current context used for drawing (between a BeginFrame/EndFrame)
	ImGuiContext*						mpContext					= nullptr;	// Context that the remote drawing should use (either the one active when connection request happened, or a clone)

#if IMGUI_HAS_CALLBACK	
	ImGuiContextHook					mImguiHookNewframe;
	ImGuiContextHook					mImguiHookEndframe;
#endif

	const void*							mpFontTextureData			= nullptr;	// Last font texture data send to server (used to detect if font was changed)
	SavedImguiContext					mSavedContextValues;
	Time								mTimeTracking;
	std::atomic_int32_t					mTexturesPendingCount;	
	float								mMouseWheelVertPrev			= 0.f;
	float								mMouseWheelHorizPrev		= 0.f;	
	bool								mbDisconnectRequest			= false;	// Waiting to Disconnect
	bool								mbHasTextureUpdate			= false;
	bool								mbIsRemoteDrawing			= false;	// True if the rendering it meant for the remote netImgui server
	bool								mbRestorePending			= false;	// Original context has had some settings overridden, original values stored in mRestoreXXX	
	bool								mbFontUploaded				= false;	// Auto detect if font was sent to server
	
	char								PADDING[7];
	void								TextureProcessPending();
	void								TextureProcessRemoval();
	inline bool							IsConnected()const;
	inline bool							IsConnectPending()const;
	inline bool							IsActive()const;
	inline void							KillSocketComs();
	inline void							KillSocketListen();
	inline void							KillSocketAll();

// Prevent warnings about implicitly created copy
protected:
	ClientInfo(const ClientInfo&)=delete;
	ClientInfo(const ClientInfo&&)=delete;
	void operator=(const ClientInfo&)=delete;
};

//=============================================================================
// Main communication thread, that should be started in its own thread
//=============================================================================
void CommunicationsClient(void* pClientVoid);
void CommunicationsHost(void* pClientVoid);

}}} //namespace NetImgui::Internal::Client

#include "NetImgui_Client.inl"
