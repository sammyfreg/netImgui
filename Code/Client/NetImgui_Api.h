#pragma once

//=================================================================================================
//! @Name		: NetImgui
//=================================================================================================
//! @author		: Sammy Fatnassi
//! @date		: 2022/01/31
//!	@version	: v1.7.5
//! @Details	: For integration info : https://github.com/sammyfreg/netImgui/wiki
//=================================================================================================
#define NETIMGUI_VERSION		"1.7.5"
#define NETIMGUI_VERSION_NUM	10705


#ifdef NETIMGUI_IMPLEMENTATION
	#define NETIMGUI_INTERNAL_INCLUDE
#endif

//-------------------------------------------------------------------------------------------------
// Deactivate a few warnings to allow Imgui header include
// without generating warnings in maximum level '-Wall'
//-------------------------------------------------------------------------------------------------
#if defined (__clang__)
	#pragma clang diagnostic push
	// ImGui.h warnings(s)
	#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
	// NetImgui_Api.h Warning(s)
	#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"	// Not using nullptr in case this file is used in pre C++11
#elif defined(_MSC_VER) 
	#pragma warning	(push)
	// ImGui.h warnings(s)
	#pragma warning (disable: 4514)		// 'xxx': unreferenced inline function has been removed
	#pragma warning (disable: 4710)		// 'xxx': function not inlined
	#pragma warning (disable: 4820)		// 'xxx': 'yyy' bytes padding added after data member 'zzz'	
	#pragma warning (disable: 5045)		// Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
#endif

//=================================================================================================
// Include the user config file. It should contain the include for :
// 'imgui.h' : always
// 'imgui_internal.h' when 'NETIMGUI_INTERNAL_INCLUDE' is defined
//=================================================================================================
#include "NetImgui_Config.h"

//-------------------------------------------------------------------------------------------------
// If 'NETIMGUI_ENABLED' hasn't been defined yet (in project settings or NetImgui_Config.h') 
// we define this library as 'Disabled'
//-------------------------------------------------------------------------------------------------
#if !defined(NETIMGUI_ENABLED)
	#define NETIMGUI_ENABLED 0
#endif

// NetImgui needs to detect Dear ImGui to be active, otherwise we disable it
// When including this header, make sure imgui.h is included first 
// (either always included in NetImgui_config.h or have it included after Imgui.h in your cpp)
#if !defined(IMGUI_VERSION)
	#undef	NETIMGUI_ENABLED
	#define	NETIMGUI_ENABLED 0
#endif

#if NETIMGUI_ENABLED

#include <stdint.h>

//-------------------------------------------------------------------------------------------------
// Prepended to functions signature, for dll export/import
//-------------------------------------------------------------------------------------------------
#ifndef NETIMGUI_API
	#define NETIMGUI_API IMGUI_API	// Use same value as defined by Dear ImGui by default 
#endif

namespace NetImgui 
{ 

//=================================================================================================
// List of texture format supported
//=================================================================================================
enum eTexFormat { 
	kTexFmtA8, 
	kTexFmtRGBA8, 
	kTexFmt_Count,
	kTexFmt_Invalid=kTexFmt_Count 
};

//=================================================================================================
// Data Compression wanted status
//=================================================================================================
enum eCompressionMode {
	kForceDisable,			// Disable data compression for communications
	kForceEnable,			// Enable data compression for communications
	kUseServerSetting		// Use Server setting for compression (default)
};

//-------------------------------------------------------------------------------------------------
// Thread start function
//-------------------------------------------------------------------------------------------------
typedef void ThreadFunctPtr(void threadedFunction(void* pClientInfo), void* pClientInfo);

//=================================================================================================
// Initialize the Network Library
//=================================================================================================
NETIMGUI_API	bool				Startup(void);

//=================================================================================================
// Free Resources
// Wait until all communication threads have terminated before returning
//=================================================================================================
NETIMGUI_API	void				Shutdown();

//=================================================================================================
// Try to establish a connection to NetImgui server application. 
//
// Can establish connection with netImgui Server application by either reaching it directly
// using 'ConnectToApp' or waiting for Server to reach us after Client called 'ConnectFromApp'.
//
// Note:	Start a new communication thread using std::Thread by default, but can receive custom 
//			thread start function instead (Look at ClientExample 'CustomCommunicationThread').
//-------------------------------------------------------------------------------------------------
// clientName		: Named that will be displayed on the Server, for this Client
// serverHost		: Address of the netImgui Server application (Ex1: 127.0.0.2, Ex2: localhost)
// serverPort		: PortID of the netImgui Server application to connect to
// clientPort		: PortID this Client should wait for connection from Server application
// threadFunction	: User provided function to launch new networking thread.
//					  Use 'DefaultStartCommunicationThread' by default, relying on 'std::thread'.
//=================================================================================================
NETIMGUI_API	bool				ConnectToApp(const char* clientName, const char* serverHost, uint32_t serverPort=kDefaultServerPort, ThreadFunctPtr threadFunction=0);
NETIMGUI_API	bool				ConnectFromApp(const char* clientName, uint32_t clientPort=kDefaultClientPort, ThreadFunctPtr threadFunction=0);

//=================================================================================================
// Request a disconnect from the netImguiApp server
//=================================================================================================
NETIMGUI_API	void				Disconnect(void);

//=================================================================================================
// True if connected to netImguiApp server
//=================================================================================================
NETIMGUI_API	bool				IsConnected(void);

//=================================================================================================
// True if connection request is waiting to be completed. For example, while waiting for  
// Server to reach ud after having called 'ConnectFromApp()'
//=================================================================================================
NETIMGUI_API	bool				IsConnectionPending(void);

//=================================================================================================
// True when Dear ImGui is currently expecting draw commands 
// This means that we are between NewFrame() and EndFrame() 
//=================================================================================================
NETIMGUI_API	bool				IsDrawing(void);

//=================================================================================================
// True when Dear ImGui is currently expecting draw commands *sent to remote netImgui app*.
// This means that we are between NewFrame() and EndFrame() of drawing for remote application
//=================================================================================================
NETIMGUI_API	bool				IsDrawingRemote(void);

//=================================================================================================
// Send an updated texture used by imgui, to netImguiApp server
// Note: To remove a texture, set pData to nullptr
//=================================================================================================
NETIMGUI_API	void				SendDataTexture(ImTextureID textureId, void* pData, uint16_t width, uint16_t height, eTexFormat format);

//=================================================================================================
// Start a new Imgui Frame and wait for Draws commands, using a ImGui created internally
// for remote drawing. Returns true if we are awaiting a new ImGui frame. 
//
// All ImGui drawing should be skipped when return is false.
//
// Note: This code can be used instead, to know if you should be drawing or not :
//			'if( !NetImgui::IsDrawing() )'
//
// Note: If your code cannot handle skipping a ImGui frame, leave 'bSupportFrameSkip=false',
//		 and this function will always call 'ImGui::NewFrame()' internally and return true
//
// Note: With Dear ImGui 1.81+, you can keep using the ImGui::BeginFrame()/Imgui::Render()
//		 without having to use NetImgui::NewFrame()/NetImgui::EndFrame() 
//		 (unless wanting to support frame skip)
//=================================================================================================
NETIMGUI_API	bool				NewFrame(bool bSupportFrameSkip=false);

//=================================================================================================
// Process all receives draws, send them to remote connection and restore the ImGui Context
//=================================================================================================
NETIMGUI_API	void				EndFrame(void);

//=================================================================================================
// Return the context associated to this remote connection. Null when not connected.
//=================================================================================================
NETIMGUI_API	ImGuiContext*		GetContext();

//=================================================================================================
// Set the remote client background color and texture
// Note: If no TextureID is specified, will use the default server texture
//=================================================================================================
NETIMGUI_API	void				SetBackground(const ImVec4& bgColor);
NETIMGUI_API	void				SetBackground(const ImVec4& bgColor, const ImVec4& textureTint );
NETIMGUI_API	void				SetBackground(const ImVec4& bgColor, const ImVec4& textureTint, ImTextureID bgTextureID);

//=================================================================================================
// Control the data compression for communications between Client/Server
//=================================================================================================
NETIMGUI_API	void				SetCompressionMode(eCompressionMode eMode);
NETIMGUI_API	eCompressionMode	GetCompressionMode();

//=================================================================================================
// Helper function to quickly create a context duplicate (sames settings/font/styles)
//=================================================================================================
NETIMGUI_API	uint8_t				GetTexture_BitsPerPixel	(eTexFormat eFormat);
NETIMGUI_API	uint32_t			GetTexture_BytePerLine	(eTexFormat eFormat, uint32_t pixelWidth);
NETIMGUI_API	uint32_t			GetTexture_BytePerImage	(eTexFormat eFormat, uint32_t pixelWidth, uint32_t pixelHeight);
} 

//=================================================================================================
// Optional single include compiling option
// Note: User that do not wish adding the few NetImgui cpp files to their project,
//		 can instead define 'NETIMGUI_IMPLEMENTATION' *once* before including 'NetImgui_Api.h'
//		 and this will load the required cpp files alongside
//=================================================================================================
#if defined(NETIMGUI_IMPLEMENTATION)
	#include "Private/NetImgui_Api.cpp"
	#include "Private/NetImgui_Client.cpp"
	#include "Private/NetImgui_CmdPackets_DrawFrame.cpp"
	#include "Private/NetImgui_NetworkPosix.cpp"
	#include "Private/NetImgui_NetworkUE4.cpp"
	#include "Private/NetImgui_NetworkWin32.cpp"
	#undef NETIMGUI_INTERNAL_INCLUDE
#endif

#endif // NETIMGUI_ENABLED

//-------------------------------------------------------------------------------------------------
// Re-Enable the Deactivated warnings
//-------------------------------------------------------------------------------------------------
#if defined (__clang__)
	#pragma clang diagnostic pop
#elif defined(_MSC_VER) 
	#pragma warning	(pop)
#endif
