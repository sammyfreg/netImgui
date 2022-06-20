#pragma once

#include <Private/NetImgui_Shared.h>
#include <Private/NetImgui_Network.h>

//=============================================================================================
// SELECT RENDERING/OS API HERE
//=============================================================================================
#define HAL_API_PLATFORM_WIN32_DX11		1
#define HAL_API_PLATFORM_GLFW_GL3		0							// Currently only compiles in release (library include compatibility)
#define HAL_API_RENDERTARGET_INVERT_Y	(HAL_API_PLATFORM_GLFW_GL3)	// Invert client render target Y axis (since OpenGL start texture UV from BottomLeft instead of DirectX TopLeft)
//=============================================================================================


namespace NetImguiServer { namespace RemoteClient { struct Client; } } // Forward declare

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
	// Add a new remote client config to our list (to attempt conexion)
	bool	AddClientConfigFromString(const char* string, bool transient);	
	
	// Descriptor of each textures by Server. Format always RGBA8
	// @sammyfreg todo: make this safer with smart pointer instead of manually deleting the HALTexture
	struct ServerTexture 
	{
		inline bool	IsValid(){ return mpHAL_Texture != nullptr; }
		void*		mpHAL_Texture		= nullptr;
		uint16_t	mSize[2]			= {0, 0};
		uint64_t	mImguiId			= 0u;		// Associated ImGui TextureId in Imgui commandlist
	};

	//=============================================================================================
	// Note:	(H)ardware (A)bstraction (L)ayer
	//			When porting the 'NetImgui Server' application to other platform, 
	//			theses are the few functions needed to be supported by each specific API that 
	//			are not already supported by de 'Dear ImGui' provided backends
	//=============================================================================================
	// Additional initialisation that are platform specific
	bool	HAL_Startup(const char* CmdLine);
	// Prepare for shutdown of application, with platform specific code
	void	HAL_Shutdown();
	// Receive a platform specific socket, and return us with info on the connection
	bool	HAL_GetSocketInfo(NetImgui::Internal::Network::SocketInfo* pClientSocket, char* pOutHostname, size_t HostNameLen, int& outPort);
	// Receive a command to execute by the OS. Used to open our weblink to the NetImgui Github
	void	HAL_ShellCommand(const char* aCommandline);
	
	// Receive a ImDrawData drawlist and request Dear ImGui's backend to output it into a texture
	void	HAL_RenderDrawData(RemoteClient::Client& client, ImDrawData* pDrawData);
	// Allocate a texture resource
	bool	HAL_CreateTexture(uint16_t Width, uint16_t Height, NetImgui::eTexFormat Format, const uint8_t* pPixelData, ServerTexture& OutTexture);
	// Free a Texture resource
	void	HAL_DestroyTexture( ServerTexture& OutTexture );
	// Allocate a RenderTarget that each client will use to output their ImGui drawing into.
	bool	HAL_CreateRenderTarget(uint16_t Width, uint16_t Height, void*& pOutRT, void*& pOutTexture );
	// Free a RenderTarget resource
	void	HAL_DestroyRenderTarget(void*& pOutRT, void*& pOutTexture );

}}
