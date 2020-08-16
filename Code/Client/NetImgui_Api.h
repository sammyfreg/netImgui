#pragma once

//=================================================================================================
//! @Name		: netImgui
//=================================================================================================
//! @author		: Sammy Fatnassi
//! @date		: 2020/07/02
//!	@version	: v1.01.001
//! @Details	: For integration info : https://github.com/sammyfreg/netImgui/wiki
//=================================================================================================
#define NETIMGUI_VERSION		"1.01.001"
#define NETIMGUI_VERSION_NUM	101001

struct ImGuiContext;
struct ImDrawData;

#include <stdint.h>

#include "Private/NetImgui_WarningDisable.h"
#include "NetImgui_Config.h"

//=================================================================================================
// List of texture format supported
//=================================================================================================
namespace NetImgui 
{ 
enum class eTexFormat : uint8_t { 
	kTexFmtA8, 
	kTexFmtRGBA8, 
	kTexFmt_Count,
	kTexFmt_Invalid=kTexFmt_Count };

typedef void		ThreadFunctPtr(void threadedFunction(void* pClientInfo), void* pClientInfo) ;

//=================================================================================================
// Initialize the Network Library
//=================================================================================================
bool				Startup(void);

//=================================================================================================
// Free Resources
//=================================================================================================
void				Shutdown(void);

//=================================================================================================
// Try to establish a connection to netImguiApp server. 
// Will create a new ImGui Context by copying the current settings
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
// bReuseLocalTime	: ImGui Time tracking comes from the original ImGui Context when true. 
//					  Otherwise use its own time. 
//					 (Helps solves issue if your program uses ImGui::GetTime() and 
//					  expect it to be continuous between local and remote Context).
//	threadFunction	: User provided function to launch a new thread runningthe function
//					  received as a parameter. Use 'DefaultStartCommunicationThread'
//					  by default, which relies on 'std::thread'.
//=================================================================================================
bool				ConnectToApp(const char* clientName, const char* serverHost, uint32_t serverPort=kDefaultServerPort, bool bReuseLocalTime=true);
bool				ConnectToApp(ThreadFunctPtr threadFunction, const char* clientName, const char* ServerHost, uint32_t serverPort = kDefaultServerPort, bool bReuseLocalTime=true);
bool				ConnectFromApp(const char* clientName, uint32_t clientPort=kDefaultClientPort, bool bReuseLocalTime=true);
bool				ConnectFromApp(ThreadFunctPtr threadFunction, const char* clientName, uint32_t serverPort=kDefaultClientPort, bool bReuseLocalTime=true);

//=================================================================================================
// Request a disconnect from the netImguiApp server
//=================================================================================================
void				Disconnect(void);

//=================================================================================================
// True if connected to netImguiApp server
//=================================================================================================
bool				IsConnected(void);

//=================================================================================================
// True if connection request is waiting to be completed. Waiting for Server to connect to us 
// after having called 'ConnectFromApp()' for example
//=================================================================================================
bool				IsConnectionPending(void);

//=================================================================================================
// True Dear ImGui is currently expecting draw commands sent to remote netImgui app.
// This means that we are between NewFrame() and EndFrame() of drawing for remote application
//=================================================================================================
bool				IsRemoteDraw(void);

//=================================================================================================
// Send an updated texture used by imgui, to netImguiApp server
// Note: To remove a texture, set pData to nullptr
//=================================================================================================
void				SendDataTexture(uint64_t textureId, void* pData, uint16_t width, uint16_t height, eTexFormat format);

//=================================================================================================
// Start a new Imgui Frame and wait for Draws commands. Sets a new current ImGui Context
//=================================================================================================
bool				NewFrame(void);

//=================================================================================================
// Process all receives draws, send them to remote connection and restore the ImGui Context
//=================================================================================================
const ImDrawData*	EndFrame(void);

//=================================================================================================
// Get Imgui drawing context used for remote Connection. 
// Usefull to tweak some style / io values
//=================================================================================================
ImGuiContext*		GetRemoteContext();

uint8_t				GetTexture_BitsPerPixel	(eTexFormat eFormat);
uint32_t			GetTexture_BytePerLine	(eTexFormat eFormat, uint32_t pixelWidth);
uint32_t			GetTexture_BytePerImage	(eTexFormat eFormat, uint32_t pixelWidth, uint32_t pixelHeight);
} 

#include "Private/NetImgui_WarningReenable.h"
