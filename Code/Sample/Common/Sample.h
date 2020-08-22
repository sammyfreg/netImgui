#pragma once

#include <stdint.h>
struct ImDrawData;	// Forward declare

namespace SampleClient
{
//-----------------------------------------------------------------------------
// Methods implemented in each samples
//-----------------------------------------------------------------------------
bool				Client_Startup();
void				Client_Shutdown();
void				Client_AddFontTexture(uint64_t texId, void* pData, uint16_t width, uint16_t height);
const ImDrawData*	Client_Draw();

//-----------------------------------------------------------------------------
// Utility methods available in Samples
//-----------------------------------------------------------------------------
void ClientUtil_ImGuiContent_Common(const char* zAppName, bool bCreateNewContext=false);

}
