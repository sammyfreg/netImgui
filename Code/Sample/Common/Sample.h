#pragma once

#include <NetImgui_Api.h>

// forward declares when NetImgui not enabled
#if !NETIMGUI_ENABLED
	struct ImDrawData;
	namespace NetImgui { using ThreadFunctPtr = void*; }
#endif

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
void ClientUtil_ImGuiContent_Common(const char* zAppName, NetImgui::ThreadFunctPtr customThreadLauncher=nullptr);

}

#include <Client/Private/NetImgui_WarningDisable.h> 