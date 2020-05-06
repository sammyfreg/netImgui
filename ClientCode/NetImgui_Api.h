#pragma once

#include <stdint.h>
struct ImDrawData;	//Forward declare of a dear imgui struct
struct ImGuiIO;		//Forward declare of a dear imgui struct

namespace NetImgui 
{ 

enum eTexFormat : uint8_t { kTexFmtR8, kTexFmtRG8, kTexFmtRGB8, kTexFmtRGBA8, kTexFmt_Invalid };

constexpr uint32_t kDefaultServerPort = 8888;

//=================================================================================================
// Initialize the Network Library, and assign custom malloc/free 
// (use std::malloc/free by default)
//=================================================================================================
bool	Startup				(void* (*pMalloc)(size_t) = nullptr, void (*pFree)(void*) = nullptr);

//=================================================================================================
// Free Resources
//=================================================================================================
void	Shutdown			(void);

//=================================================================================================
// Try to establish a connection to netImguiApp server
//=================================================================================================
bool	Connect				(ImGuiIO& imguiIO, const char* clientName, const uint8_t serverIp[4], uint32_t serverPort=kDefaultServerPort);

//=================================================================================================
// Request a disconnect from the netImguiApp server
//=================================================================================================
void	Disconnect			(void);

//=================================================================================================
// True if connected to netImguiApp server
//=================================================================================================
bool	IsConnected			(void);

//=================================================================================================
// Send the latest result of Imgui Draw Data, to netImguiApp server
//=================================================================================================
void	SendDataDraw		(const ImDrawData* pImguiDrawData);

//=================================================================================================
// Send an updated texture used by imgui, to netImguiApp server
//=================================================================================================
void	SendDataTexture		(uint64_t textureId, void* pData, uint16_t width, uint16_t height, eTexFormat format);

//=================================================================================================
// Update Imgui with the latest keyboard/mouse/screen informations. 
// If there's new data, a rendering update should be sent back to netImguiApp server with
// 'SendDataDraw'
//=================================================================================================
bool	InputUpdateData		(void);

uint8_t		GetTexture_BitsPerPixel	(eTexFormat eFormat);
uint32_t	GetTexture_BytePerLine	(eTexFormat eFormat, uint32_t pixelWidth);
uint32_t	GetTexture_BytePerImage	(eTexFormat eFormat, uint32_t pixelWidth, uint32_t pixelHeight);
} 
