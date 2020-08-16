#pragma once

#include <stdint.h>
struct ImDrawData;	// Forward declare
struct ImVec4;		// Forward declare

namespace SampleClient
{
bool		Client_Startup();
void		Client_Shutdown();
void		Client_AddFontTexture(uint64_t texId, void* pData, uint16_t width, uint16_t height);
ImDrawData*	Client_Draw();
}
