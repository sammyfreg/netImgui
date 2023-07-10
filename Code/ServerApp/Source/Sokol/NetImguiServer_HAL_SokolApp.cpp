#include "NetImguiServer_App.h"

#if HAL_API_PLATFORM_SOKOL
#include <Private/NetImgui_Shared.h>


namespace NetImguiServer { namespace App
{

//=================================================================================================
// HAL STARTUP
// Additional initialisation that are platform specific
//=================================================================================================
bool HAL_Startup(const char* CmdLine)
{
	IM_UNUSED(CmdLine);
	return true;
}

//=================================================================================================
// HAL SHUTDOWN
// Prepare for shutdown of application, with platform specific code
//=================================================================================================
void HAL_Shutdown()
{

}

//=================================================================================================
// HAL GET SOCKET INFO
// Take a platform specific socket (based on the NetImguiNetworkXXX.cpp implementation) and
// fetch informations about the client IP connected
//=================================================================================================
bool HAL_GetSocketInfo(NetImgui::Internal::Network::SocketInfo* pClientSocket, char* pOutHostname, size_t HostNameLen, int& outPort)
{
	return false;
}
}} // namespace NetImguiServer { namespace App

#endif // HAL_API_PLATFORM_SOKOL