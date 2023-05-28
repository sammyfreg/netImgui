#include "NetImguiServer_App.h"

#if HAL_API_PLATFORM_GLFW_GL3



#include <Private/NetImgui_Shared.h>

//=================================================================================================
// WINDOWS GLFW
// Note: This file currently only has support for Windows application
#if _WIN32
	extern int main(int, char**);
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif

	#include <windows.h>
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <shellapi.h>	// To open webpage link
	#include "../resource.h"

	// WIN MAIN
	// Main window method expected as as Win32 Application. 
	// Original 'main' method used by Dear ImGui sample is intended for a console Application, so we 
	// need to provided expected entry point and manually call main.
	int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
	{
		IM_UNUSED(hInstance);
		IM_UNUSED(hPrevInstance);
		IM_UNUSED(lpCmdLine);
		IM_UNUSED(nCmdShow);
		return main(0, nullptr);
	}
#endif // _WIN32
//=================================================================================================


namespace NetImguiServer { namespace App
{

//=================================================================================================
// HAL STARTUP
// Additional initialisation that are platform specific
//=================================================================================================
bool HAL_Startup(const char* CmdLine)
{
	IM_UNUSED(CmdLine);

#if _WIN32
	// Change the icon for hwnd's window class.
	HICON appIconBig				= LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_NETIMGUIAPP));
	HICON appIconSmall				= LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_SMALL));
	ImGuiViewport* main_viewport	= ImGui::GetMainViewport();
    HWND hwnd						= reinterpret_cast<HWND>(main_viewport->PlatformHandleRaw);
	SendMessage(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(appIconBig));
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(appIconSmall));
#endif
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
#if _WIN32
	sockaddr socketAdr;
	int sizeSocket(sizeof(sockaddr));
	SOCKET* pClientSocketWin = reinterpret_cast<SOCKET*>(pClientSocket);
	if( getsockname(*pClientSocketWin, &socketAdr, &sizeSocket) == 0 )
	{
		char zPortBuffer[32];
		if( getnameinfo(&socketAdr, sizeSocket, pOutHostname, static_cast<DWORD>(HostNameLen), zPortBuffer, sizeof(zPortBuffer), NI_NUMERICSERV) == 0 )
		{
			outPort = atoi(zPortBuffer);
			return true;
		}
	}
#endif
	return false;
}
}} // namespace NetImguiServer { namespace App

#endif // HAL_API_PLATFORM_WIN32DX11