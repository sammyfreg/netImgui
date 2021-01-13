#pragma once

#include <Private/NetImgui_Shared.h>
#include <Private/NetImgui_Network.h>

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

	//=============================================================================================
	// Note:	(H)ardware (A)bstraction (L)ayer
	//			When porting the 'NetImgui Server' application to other platform, 
	//			theses are the few functions needed to be supported by each specific API that 
	//			are not already supported by de 'Dear ImGui' provided backends
	//=============================================================================================
	// Receive the 'Dear ImGui' key down values from the 'NetImgui' application, and convert them to windows virtual key codes
	void	HAL_ConvertKeyDown(const bool ImguiKeysDown[512], uint64_t outKeysDownMask[512/64] );
	// Receive a platform specific socket, and return us with info on the connection
	bool	HAL_GetSocketInfo(NetImgui::Internal::Network::SocketInfo* pClientSocket, char* pOutHostname, size_t HostNameLen, int& outPort);
	// Receive a command to execute by the OS. Used to open our weblink to the NetImgui Github
	void	HAL_ShellCommand(const char* aCommandline);

	// During init, allows to initialize some additiona rendering settings 
	void	HAL_RenderStateSetup();
	// Receive a ImDrawData drawlist and request Dear ImGui's backend to output it into a texture
	void	HAL_RenderDrawData(void* pRT, ImDrawData* pDrawData, const float ClearColor[4]);	
	// Allocate a texture resource
	bool	HAL_CreateTexture(uint16_t Width, uint16_t Height, NetImgui::eTexFormat Format, const uint8_t* pPixelData, void*& pOutTexture);
	// Free a Texture resource
	void	HAL_DestroyTexture( void*& pTexture );
	// Allocate a RenderTarget that each client will use to output their ImGui drawing into.
	bool	HAL_CreateRenderTarget(uint16_t Width, uint16_t Height, void*& pOutRT, void*& pOutTexture );
	// Free a RenderTarget resource
	void	HAL_DestroyRenderTarget(void*& pOutRT, void*& pOutTexture );

}}
