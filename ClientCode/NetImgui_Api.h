#pragma once

// Note: Tested against 'dear imgui' v1.77, but other versions can still be compatible
// Note: Use memory allocator assigned to ImGui

#include <imgui.h>		// Set path to your 'dear imgui' header here
#include <stdint.h>

namespace NetImgui 
{ 

enum eTexFormat : uint8_t { kTexFmtR8, kTexFmtRG8, kTexFmtRGB8, kTexFmtRGBA8, kTexFmt_Invalid };

constexpr uint32_t kDefaultServerPort = 8888;

//=================================================================================================
// Initialize the Network Library
//=================================================================================================
bool		Startup				(void);

//=================================================================================================
// Free Resources
//=================================================================================================
void		Shutdown			(void);

//=================================================================================================
// Try to establish a connection to netImguiApp server. 
// Will create a new ImGui Context by copying the current settings
//=================================================================================================
bool		Connect				(const char* clientName, const uint8_t serverIp[4], uint32_t serverPort=kDefaultServerPort);

//=================================================================================================
// Request a disconnect from the netImguiApp server
//=================================================================================================
void		Disconnect			(void);

//=================================================================================================
// True if connected to netImguiApp server
//=================================================================================================
bool		IsConnected			(void);

//=================================================================================================
// Send an updated texture used by imgui, to netImguiApp server
//=================================================================================================
void		SendDataTexture		(uint64_t textureId, void* pData, uint16_t width, uint16_t height, eTexFormat format);

//=================================================================================================
// Start a new Imgui Frame and wait for Draws commands. Sets a new current ImGui Context
//=================================================================================================
bool		NewFrame			(void);

//=================================================================================================
// Process all receives draws, send them to remote connection and restore the ImGui Context
//=================================================================================================
void		EndFrame			(void);

uint8_t		GetTexture_BitsPerPixel	(eTexFormat eFormat);
uint32_t	GetTexture_BytePerLine	(eTexFormat eFormat, uint32_t pixelWidth);
uint32_t	GetTexture_BytePerImage	(eTexFormat eFormat, uint32_t pixelWidth, uint32_t pixelHeight);
} 
