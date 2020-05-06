#pragma once

#include <stdint.h>
#include <wtypes.h>
struct ImDrawData;	// Forward declare
struct ImVec4;		// Forward declare

namespace SampleClient
{
bool	Client_Startup();
void	Client_Shutdown();
bool	Client_SetImguiContextRemote();
bool	Client_SetImguiContextLocal();
void	Client_AddFontTexture(uint64_t texId, void* pData, uint16_t width, uint16_t height);

void	Client_DrawLocal(ImVec4& clearColorOut);
void	Client_DrawRemote(ImVec4& clearColorOut);

void	Imgui_DrawMainMenu();
void	Imgui_DrawContent(ImVec4& clear_col);
void	Imgui_DrawContentSecondary();

}
