#pragma once
#include <stdint.h>

// Note: Use memory allocator assigned to ImGui

//=================================================================================================
// Set the path to 'imgui.h' used by your codebase here. 
// Also suppress a few warnings imgui.h generates in 'warning All' (-Wall)
//=================================================================================================
#include "Private/NetImgui_WarningDisableImgui.h" 
#include <Imgui.h>
#include "Private/NetImgui_WarningReenable.h"

//=================================================================================================
// Enable code compilation for this library
// Note: Usefull to turn off 'netImgui' on unsupported build targets.
//=================================================================================================
#ifndef NETIMGUI_ENABLED
#define NETIMGUI_ENABLED	1
#endif


//=================================================================================================
// Enable default Win32 networking code
// Note:	Usefull to turn off and implement your own implementation. 
//			If disabled, then functions implementng 'NetImgui_Network.h' need to be provided
//			If enabled, then 'ws2_32.lib' library need to be included in project input
//=================================================================================================
#ifndef NETIMGUI_WINSOCKET_ENABLED
#define NETIMGUI_WINSOCKET_ENABLED	(_WIN32 && 1)
#endif

//=================================================================================================
// List of texture format supported
//=================================================================================================
namespace NetImgui 
{ 
enum eTexFormat { 
	/*kTexFmtR8, kTexFmtRG8, kTexFmtRGB8,*/ //SF To implement
	kTexFmtRGBA8, kTexFmt_Invalid };

//=================================================================================================
// Port used by connect the Server and Client together
//=================================================================================================
enum eConstants{
	kDefaultServerPort = 8888
};

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
// Note:	Start a new communication thread using std::Thread by default, but can receive custom 
//			thread start function instead. Look at ClientExample 'CustomCommunicationThread' 
//=================================================================================================
typedef void StartThreadFunctPtr(void CommunicationLoop(void* pClientInfo), void* pClientInfo) ;
bool				Connect(const char* clientName, const char* ServerHost, uint32_t serverPort=kDefaultServerPort, bool bReuseLocalTime=true);
bool				Connect(StartThreadFunctPtr startThreadFunction, const char* clientName, const char* ServerHost, uint32_t serverPort = kDefaultServerPort, bool bReuseLocalTime=true);

//=================================================================================================
// Request a disconnect from the netImguiApp server
//=================================================================================================
void				Disconnect(void);

//=================================================================================================
// True if connected to netImguiApp server
//=================================================================================================
bool				IsConnected(void);

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

