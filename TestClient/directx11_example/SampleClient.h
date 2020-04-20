#pragma once

struct ImDrawData;	// Forward declare
struct ImVec4;		// Forward declare

namespace SampleClient
{
bool Startup();
void Shutdown();
void Imgui_DrawMainMenu();
void Imgui_DrawWindows(ImVec4& clear_col);
void Imgui_DrawCallback(ImDrawData* pImguiDrawData);
}
