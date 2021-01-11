#pragma once

#include <stdint.h>
struct ImDrawData;	// Forward declare

namespace SampleClient
{
//-----------------------------------------------------------------------------
// Methods implemented in each samples
//-----------------------------------------------------------------------------
bool			Client_Startup();
void			Client_Shutdown();
ImDrawData*		Client_Draw();

//-----------------------------------------------------------------------------
// Utility methods available in Samples
//-----------------------------------------------------------------------------
void ClientUtil_ImGuiContent_Common(const char* zAppName);

}

#include <Client/Private/NetImgui_WarningDisable.h> 