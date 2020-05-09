#pragma once

#include <vector>
#include "NetImGui_Shared.h"

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
	inline		~ClientTexture();
	inline void	Set( CmdTexture* pCmdTexture );
	inline bool	IsValid()const;
	bool		mbSent		= false;
	CmdTexture* mpCmdTexture= nullptr;
};

//=============================================================================
// Keep all Client infos needed for communication with server
//=============================================================================
struct ClientInfo
{
using VecTexture	= std::vector<ClientTexture, stdAllocator<ClientTexture> >;
using BufferKeys	= Ringbuffer<uint16_t, 1024>;
using Time			= std::chrono::time_point<std::chrono::high_resolution_clock>;

Network::SocketInfo*		mpSocket			= nullptr;
char						mName[16]			= {0};
VecTexture					mTextures;
CmdTexture*					mTexturesPending[16];
std::atomic_int32_t			mTexturesPendingCount = 0;
ExchangePtr<CmdDrawFrame>	mPendingFrameOut;
ExchangePtr<CmdInput>		mPendingInputIn;
BufferKeys					mPendingKeyIn;
bool						mbConnected			= false;
bool						mbDisconnectRequest	= false;
bool						mbHasTextureUpdate	= false;
ImGuiContext*				mpContext			= nullptr;
ImGuiContext*				mpContextRestore	= nullptr;
float						mMouseWheelVertPrev	= 0.f;
float						mMouseWheelHorizPrev= 0.f;
void						ProcessTextures();
};

//=============================================================================
// Main communication thread, that should be started in its own thread
//=============================================================================
void Communications(ClientInfo* pClient);

}}} //namespace NetImgui::Internal::Client

#include "NetImGui_Client.inl"