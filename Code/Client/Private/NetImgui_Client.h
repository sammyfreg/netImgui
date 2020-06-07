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
	uint8_t		mPadding[7];
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
	Network::SocketInfo*		mpSocket				= nullptr;
	char						mName[16]				={0};
	VecTexture					mTextures;
	CmdTexture*					mTexturesPending[16];
	ExchangePtr<CmdDrawFrame>	mPendingFrameOut;
	ExchangePtr<CmdInput>		mPendingInputIn;
	BufferKeys					mPendingKeyIn;
	ImGuiContext*				mpContext				= nullptr;
	ImGuiContext*				mpContextRestore		= nullptr;	// Context to restore to Imgui once drawing is done
	std::atomic_int32_t			mTexturesPendingCount;
	float						mMouseWheelVertPrev		= 0.f;
	float						mMouseWheelHorizPrev	= 0.f;
	bool						mbConnected				= false;	// Sucessfully Connected
	bool						mbDisconnectRequest		= false;	// Waiting to Disconnect
	bool						mbConnectRequest		= false;	// Waiting to Connect
	bool						mbHasTextureUpdate		= false;
	bool						mbReuseLocalTime		= true;		// True if the netImgui client use the original Imgui Context Time, else we will track it automatically
	char						PADDING[7];
	void						TextureProcessPending();
	void						TextureProcessRemoval();

// Prevent warnings about implicitly created copy
protected:
	ClientInfo(const ClientInfo&)=delete;
	ClientInfo(const ClientInfo&&)=delete;
	void operator=(const ClientInfo&)=delete;
};

//=============================================================================
// Main communication thread, that should be started in its own thread
//=============================================================================
void Communications(void* pClientVoid);

}}} //namespace NetImgui::Internal::Client

#include "NetImgui_Client.inl"
