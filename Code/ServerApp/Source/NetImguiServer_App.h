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
	//SF TODO DESCRIPTION
	void	LoadFonts();
	// Check for monitor DPI update and regenerate font when needed 
	// (return true when font texture needs to be created)
	void	UpdateFonts();
	// Descriptor of each textures by Server. Format always RGBA8
	// @sammyfreg todo: make this safer with smart pointer instead of manually deleting the HALTexture
	struct ServerTexture 
	{
		inline bool	IsValid(){ return mTexData.Status == ImTextureStatus_OK; }
		//void*			mpHAL_Texture		= nullptr;
		ServerTexture*	mpDeleteNext		= nullptr;	// Used to insert in a list of textures to be deleted next frame
		//uint64_t		mImguiId			= 0u;		// Associated ImGui TextureId in Imgui commandlist
		//ImTextureID		mTexID;
		uint64_t		mCustomData			= 0u;		// Memory available to custom command
		uint8_t			mIsCustom			= 0u;		// Format handled by custom version of NetImguiServer modified by library user
		uint8_t			mIsUpdatable		= 0u;		// True when textures can be updated (font)
		uint8_t			mPadding[1]			= {};
		ImTextureData	mTexData;
		//uint16_t		mSize[2]			= {0,0};
		//uint8_t			mFormat				= 0;
		//uint8_t			mPadding[3]			= {};
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
	void	EnqueueHALTextureDestroy(ServerTexture& serverTexture);
	void	CompleteHALTextureDestroy();

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
	void	HAL_UpdateTexture(ImTextureData* ImguiTextureData);
	// Allocate a RenderTarget that each client will use to output their ImGui drawing into.
	bool	HAL_CreateRenderTarget(uint16_t Width, uint16_t Height, void*& pOutRT, void*& pOutTexture );
	// Free a RenderTarget resource
	void	HAL_DestroyRenderTarget(void*& pOutRT, void*& pOutTexture );
}}
