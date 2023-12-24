#include "NetImguiServer_App.h"

#if HAL_API_PLATFORM_GLFW_GL3

#include <Private/NetImgui_Shared.h>

//=================================================================================================
// WINDOWS GLFW
// Note: This file currently only has support for Windows application
#ifdef _WIN32
	extern int main(int, char**);
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif

	#include <windows.h>
	#include <tchar.h>
	#include <shlobj_core.h> 
	#include <WinSock2.h>
	#include <WS2tcpip.h>
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

#ifdef _WIN32
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
#ifdef _WIN32
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

//=================================================================================================
// HAL GET USER SETTING FOLDER
// Request the directory where to the 'shared config' clients should be saved
// Return 'nullptr' to disable this feature
//=================================================================================================
const char* HAL_GetUserSettingFolder()
{
#ifdef _WIN32
	static char sUserSettingFolder[1024]={};
	if(sUserSettingFolder[0] == 0)
	{
		WCHAR* UserPath;
		HRESULT Ret = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &UserPath);
		if( SUCCEEDED(Ret) ){
			sprintf_s(sUserSettingFolder, "%ws\\NetImgui", UserPath); // convert from wchar to char
			DWORD ftyp = GetFileAttributesA(sUserSettingFolder);
			if (ftyp != INVALID_FILE_ATTRIBUTES && (ftyp & FILE_ATTRIBUTE_DIRECTORY) ){
				return sUserSettingFolder;
			}
			else if (ftyp == INVALID_FILE_ATTRIBUTES && CreateDirectoryA(sUserSettingFolder, nullptr) ){
				return sUserSettingFolder;
			}
		}
		sUserSettingFolder[0] = 0;
		return nullptr;
	}
	return sUserSettingFolder;
#else
	return nullptr;
#endif
}

//=================================================================================================
// HAL GET CLIPBOARD UPDATED
// Detect when clipboard had a content change and we should refetch it on the Server and
// forward it to the Clients
// 
// Note: We rely on Dear ImGui for Clipboard Get/Set but want to avoid constantly reading then
// converting it to a UTF8 text. If the Server platform doesn't support tracking change, 
// return true. If the Server platform doesn't support any clipboard, return false;
//=================================================================================================
bool HAL_GetClipboardUpdated()
{	
#ifdef _WIN32
	static DWORD sClipboardSequence(0);
	DWORD clipboardSequence = GetClipboardSequenceNumber();
	if (sClipboardSequence != clipboardSequence){
		sClipboardSequence = clipboardSequence;
		return true;
	}
#else
	// Update Clipboard content every second
	static std::chrono::steady_clock::time_point sLastCheck = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	if( (now - sLastCheck) > std::chrono::seconds(1) )
	{
		sLastCheck = now;
		return true;
	}
#endif
	return false;
}

}} // namespace NetImguiServer { namespace App

#endif // HAL_API_PLATFORM_WIN32DX11