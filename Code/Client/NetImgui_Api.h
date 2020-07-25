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

#include "NetImgui_Config.h"
#include "Private/NetImgui_WarningDisable.h"

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

#include "Private/NetImgui_WarningReenable.h"
