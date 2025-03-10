#pragma once

#include <Private/NetImgui_Shared.h>
#include <Private/NetImgui_Network.h>

//=============================================================================================
// SELECT RENDERING/OS API HERE
//=============================================================================================
#define HAL_API_PLATFORM_WIN32_DX11		1
#define HAL_API_PLATFORM_GLFW_GL3		0														// Currently only compiles in release (library include compatibility)
#define HAL_API_PLATFORM_SOKOL			0
#define HAL_API_RENDERTARGET_INVERT_Y	(HAL_API_PLATFORM_GLFW_GL3 || HAL_API_PLATFORM_SOKOL)	// Invert client render target Y axis (since OpenGL start texture UV from BottomLeft instead of DirectX TopLeft)
//=============================================================================================

// Forward declare
namespace NetImguiServer { namespace RemoteClient { struct Client; } }
namespace NetImgui { namespace Internal { struct CmdTexture; } }

namespace NetImguiServer { namespace App
{
	//=============================================================================================
	// Code specific to 'NetImgui Server' application and needed inside platform specific code
	//=============================================================================================
	// Additional initialisation needed by 'NetImGui Server' and not part of default ImGui sample code
	bool	Startup(const char* CmdLine);
	// Prepare for shutdown of application
	void	Shutdown();
	// Receive rendering request of each Remote client and output it to their own RenderTarget
	void	UpdateRemoteContent();
	// Add a new remote client config to our list (to attempt connexion)
	bool	AddTransientClientConfigFromString(const char* string);
	// Initialize the font atlas used by the Serve
	void	LoadFonts();
	// Check for monitor DPI update and regenerate font when needed 
	// (return true when font texture needs to be created)
	void	UpdateFonts();
	// Descriptor of each textures by Server. Format always RGBA8
	// @sammyfreg todo: make this safer with smart pointer instead of manually deleting the HALTexture
	struct ServerTexture 
	{
		inline ServerTexture(){ mTexData.Status = ImTextureStatus_Destroyed; }
		inline bool	IsValid(){ return mTexData.Status == ImTextureStatus_OK; }
		inline ImTextureID GetTexID()const { return mTexData.GetTexID(); }

		mutable ImTextureData	mTexData;					// Struct used by backend for texture support
		ImTextureUserID			mTexClientID;				// Client UserID associated with this texture
		uint64_t				mCustomData			= 0u;	// Memory available to custom command
		uint8_t					mIsCustom			= 0u;	// Format handled by custom version of NetImguiServer modified by library user
		uint8_t					mIsUpdatable		= 0u;	// True when textures can be updated (font)
		uint8_t					mIsReferenced		= 0u;	// True when active DrawData is using this texture
		uint8_t 				mIsPendingDel		= 0u;	// Was requested to be deleted once not in use
		uint8_t					mPadding[4]			= {};
	};

	//=============================================================================================
	// Handling of texture data
	//=============================================================================================
	bool	CreateTexture(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize);
	void	DestroyTexture(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize);

	// Library users can implement their own texture format (on client/server). Useful for vidoe streaming, new format, etc.
	bool	CreateTexture_Custom(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize);
	bool	DestroyTexture_Custom(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize);

	// Texture destruction is postponed until the end of the frame update to avoid rendering issues
	//SF TODO This needs rework to handle tracking in used texture created by custom
	void	EnqueueHALTextureDestroy(ServerTexture& serverTexture);


	//=============================================================================================
	// Note (H)ardware (A)bstraction (L)ayer
	//		When porting the 'NetImgui Server' application to other platform, 
	//		theses are the few functions needed to be supported by each specific API that 
	//		are not already supported by de 'Dear ImGui' provided backends
	//=============================================================================================
	// Additional initialisation that are platform specific
	bool	HAL_Startup(const char* CmdLine);
	// Prepare for shutdown of application, with platform specific code
	void	HAL_Shutdown();
	// Receive a platform specific socket, and return us with info on the connection
	bool	HAL_GetSocketInfo(NetImgui::Internal::Network::SocketInfo* pClientSocket, char* pOutHostname, size_t HostNameLen, int& outPort);
	// Provide the current user setting folder (used to save the shared config file)
	const char* HAL_GetUserSettingFolder();
	// Return true when new content should be retrieved from Clipboard (avoid constantly reading/converting content)
	bool	HAL_GetClipboardUpdated();
	// Receive a ImDrawData drawlist and request Dear ImGui's backend to output it into a texture
	void	HAL_RenderDrawData(RemoteClient::Client& client, ImDrawData* pDrawData);
	// Request texture updating (create/update/destroy)
	void	HAL_UpdateTexture(ImTextureData& ImguiTextureData);
	// Allocate a RenderTarget that each client will use to output their ImGui drawing into.
	bool	HAL_CreateRenderTarget(uint16_t Width, uint16_t Height, void*& pOutRT, void*& pOutTexture );
	// Free a RenderTarget resource
	void	HAL_DestroyRenderTarget(void*& pOutRT, void*& pOutTexture );
}}
