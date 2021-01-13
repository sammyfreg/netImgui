#include "NetImguiServer_App.h"
#include <Private/NetImgui_Shared.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tchar.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <shellapi.h>	// To open webpage link
#include "imgui.h"
#include "imgui_impl_win32.h"

constexpr char kServerNamedPipe[] = "\\\\.\\pipe\\netImgui";	// Communication piped to receive connection request

//=================================================================================================
// NETIMGUI NAMEDPIPE SEND MESSAGE
// Open an inter-process communication channel, detecting another instance of netImgui server
// already running and forwarding any commandline option to it.
//
// return true if there's already an instance running
//------------------------------------------------------------------------------------------------
//
// Note 1:
//	This is unrelated to communications with NetImgui Clients for Imgui Content. This allows
//	other Windows tools to request a connection between this netImgui server and a netImgui client
//------------------------------------------------------------------------------------------------
//
// Note 2:
//	This is a good example on how to add code in your own Windows tool, for a establishing a
//	connection between this netImguiServer and a netImgui client.
//=================================================================================================
bool NetImguiNamedPipeMsgSend( const char* cmdline )
{
	HANDLE hNamedPipe = CreateFileA(kServerNamedPipe, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hNamedPipe != INVALID_HANDLE_VALUE)
	{
		// Connected to named pipe, meaning there's already a running copy of netImgui Server
		// Forward commandline options.
		if( cmdline && cmdline[0] != 0 )
			WriteFile(hNamedPipe, cmdline, static_cast<DWORD>(strlen(cmdline) + 1), nullptr, nullptr);		
		CloseHandle(hNamedPipe);
		return true;
	}
	return false;
}

//=================================================================================================
// NETIMGUI NAMEDPIPE RECEIVE MESSAGE
// Receive Client connection information from another running application and 
// keep the config to attempt connexion to it later
//=================================================================================================
void NetImguiNamedPipeMsgReceive( HANDLE& hNamedPipe )
{
	char ComPipeBuffer[256];
	DWORD cbBytesRead(0);
	
	// Interprocess NamedPipe not opened for communications yet, create it
	if (hNamedPipe == INVALID_HANDLE_VALUE)
	{
		hNamedPipe = CreateNamedPipeA(kServerNamedPipe,
			PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_NOWAIT,
			1, 0, sizeof(ComPipeBuffer), 0, nullptr);
	}

	// See if there's new data waiting for us
	if( ReadFile(hNamedPipe, ComPipeBuffer, sizeof(ComPipeBuffer) - 1, &cbBytesRead, nullptr) )
	{
		DWORD cbBytesReadTotal(cbBytesRead);
		// Someone sent something, read entire data stream
		while(	cbBytesReadTotal < sizeof(ComPipeBuffer)-1 && 
				ReadFile(hNamedPipe, &ComPipeBuffer[cbBytesReadTotal], sizeof(ComPipeBuffer) - cbBytesReadTotal - 1, &cbBytesRead, nullptr) )
			cbBytesReadTotal += cbBytesRead;
		
		ComPipeBuffer[cbBytesReadTotal] = 0;
        NetImguiServer::App::AddClientConfigFromString(ComPipeBuffer, true);
		CloseHandle(hNamedPipe); // Will recreate the pipe, since it seems only 1 per remote application can exist
		hNamedPipe = INVALID_HANDLE_VALUE;
	}
}

//=================================================================================================
// WIN MAIN
// Main window method expected as as Win32 Application. 
// Original 'main' method used by Dear ImGui sample is intended for a console Application, so we 
// need to provided expected entry point and manually call main.
//=================================================================================================
extern int main(int, char**);
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

    //------------------------------------------------------------------------------------------------
	// Detect if there's another instance of the netImgui application running
	// (We only want 1 running instance, thus we forward the commandline options and close this one)
	//------------------------------------------------------------------------------------------------
	char commandline[256];
	HANDLE hInterProcessPipe(INVALID_HANDLE_VALUE);
	wcstombs_s(nullptr, commandline, sizeof(commandline), lpCmdLine, sizeof(commandline));
	if( NetImguiNamedPipeMsgSend(commandline) )
		return false;

    // Call original 'main' method
	int returnVal = main(0, nullptr);
    
	CloseHandle(hInterProcessPipe);

    return returnVal;
}

namespace NetImguiServer { namespace App
{

//=================================================================================================
// HAL CONVERT KEY DOWN
// Receive platform specific 'key down' status from ImGui, and convert them to 'Windows' specific
// key code and store them has bitmask entry that InputCmd in forwardit it to remote client.
//=================================================================================================
void HAL_ConvertKeyDown(const bool ImguiKeysDown[512], uint64_t outKeysDownMask[512/64] )
{
    // NetImgui is relying on Windows key code and this function is evaluated under windows, 
    // so no conversion key code needed, can just store they keys as bitmask directly
    for (uint64_t i(0); i < 512; ++i)
    {
        const uint64_t keyEntryIndex	= static_cast<uint64_t>(i) / 64;
	    const uint64_t keyBitMask	    = static_cast<uint64_t>(1) << static_cast<uint64_t>(i) % 64;	
        outKeysDownMask[keyEntryIndex]  = ImguiKeysDown[i] ?    outKeysDownMask[keyEntryIndex] | keyBitMask :
                                                                outKeysDownMask[keyEntryIndex] & ~keyBitMask;
    }
}

//=================================================================================================
// HAL SHELL COMMAND
// Receive a command to execute by the OS. Used to open our weblink to the NetImgui Github
//=================================================================================================
void HAL_ShellCommand(const char* aCommandline)
{
	ShellExecuteA(0, 0, aCommandline, 0, 0 , SW_SHOW );
}

//=================================================================================================
// HAL GET SOCKET INFO
// Take a platform specific socket (based on the NetImguiNetworkXXX.cpp implementation) and
// fetch informations about the client IP connected
//=================================================================================================
bool HAL_GetSocketInfo(NetImgui::Internal::Network::SocketInfo* pClientSocket, char* pOutHostname, size_t HostNameLen, int& outPort)
{
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
	return false;
}
}} // namespace NetImguiServer { namespace App
