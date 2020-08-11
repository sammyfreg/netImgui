//=================================================================================================
// Application.cpp
//
// Entry point of this application. 
// Initialize the app & window, and run the mainloop with messages processing
//=================================================================================================
#include "stdafx.h"
#include <array>
#include <chrono>
#include <iostream>

#include "../resource.h"
#include "../DirectX/DirectX11.h"
#include "ServerNetworking.h"
#include "ServerInfoTab.h"
#include "ClientRemote.h"
#include "ClientConfig.h"

constexpr uint32_t kLoadStringMax		= 100;
constexpr uint32_t kClientCountMax		= 8;
constexpr uint32_t kUIClientIFirstID	= 1000;
constexpr uint32_t kUIClientILastID		= (kUIClientIFirstID + kClientCountMax - 1);

constexpr char kServerNamedPipe[]		= "\\\\.\\pipe\\netImgui";	// Communication piped to receive connection request

// Global Variables:
static HINSTANCE		ghApplication;							// Current instance
static HWND				ghMainWindow;							// Main Windows
static WCHAR			szTitle[kLoadStringMax];				// The title bar text
static WCHAR			szWindowClass[kLoadStringMax];			// the main window class name
static InputUpdate		gAppInput;								// Input capture in Window message loop and waiting to be sent to remote client
static uint32_t			gActiveClient		= ClientRemote::kInvalidClient;	// Currently selected remote Client for input & display
static LPTSTR			gMouseCursor		= nullptr;			// Last mouse cursor assigned by received drawing command
static bool				gMouseCursorOwner	= false;			// Keep track if cursor is over Client drawing region, and thus should have its cursor updated

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void UpdateActiveClient(uint32_t NewClientId)
{
	if( NewClientId != gActiveClient )
	{
		char zName[256];
		if( gActiveClient != ClientRemote::kInvalidClient)
		{
			ClientRemote& client = ClientRemote::Get(gActiveClient);
			sprintf_s(zName, "    %s    ", client.mName);
			ModifyMenuA(GetMenu(ghMainWindow), static_cast<UINT>(client.mMenuId), MF_BYCOMMAND|MF_ENABLED, client.mMenuId, zName);
			client.mbIsActive = false;
		}
		
		gActiveClient = NewClientId;
		if( gActiveClient != ClientRemote::kInvalidClient)
		{
			ClientRemote& client = ClientRemote::Get(gActiveClient);
			sprintf_s(zName, "--[ %s ]--", client.mName);
			ModifyMenuA(GetMenu(ghMainWindow), static_cast<UINT>(client.mMenuId), MF_BYCOMMAND|MF_GRAYED, client.mMenuId, zName);
			client.mbIsActive = true;
		}		
		DrawMenuBar(ghMainWindow);
	}
}

void AddRemoteClient(uint32_t NewClientIndex)
{	
	ClientRemote& client = ClientRemote::Get(NewClientIndex);
	client.mMenuId = static_cast<uint64_t>(kUIClientIFirstID) + static_cast<uint64_t>(NewClientIndex);
	AppendMenuA(GetMenu(ghMainWindow), MF_STRING, client.mMenuId, "");
	UpdateActiveClient(NewClientIndex);
}

void RemoveRemoteClient(uint32_t OldClientIndex)
{
	ClientRemote& client = ClientRemote::Get(OldClientIndex);
	if( client.mMenuId != 0 ) // Client 0 is the ServerTab and cannot be removed
	{
		RemoveMenu(GetMenu(ghMainWindow), static_cast<UINT>(client.mMenuId), MF_BYCOMMAND);
		client.Reset();
		if( OldClientIndex == gActiveClient )
			UpdateActiveClient(0);
	}
}

BOOL Startup(HINSTANCE hInstance, int nCmdShow)
{
    // Initialize global strings	
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, kLoadStringMax);
    LoadStringW(hInstance, IDC_NETIMGUIAPP, szWindowClass, kLoadStringMax);
    WNDCLASSEXW wcex;
    wcex.cbSize			= sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NETIMGUIAPP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_NETIMGUIAPP);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	if( !RegisterClassExW(&wcex) )
		return false;
	
	ghApplication		= hInstance;
	ghMainWindow		= CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, ghApplication, nullptr);
			
	if (!ghMainWindow)
		return false;

	if( !ClientRemote::Startup(kClientCountMax) )
		return false;

	if( !dx::Startup(ghMainWindow) )
		return false;

	memset(&gAppInput, 0, sizeof(gAppInput));
	ShowWindow(ghMainWindow, nCmdShow);
	UpdateWindow(ghMainWindow);
	return true;
}

//=================================================================================================
// WNDPROC
// Process windows messages
// Mostly insterested in active 'tab' change and inputs ('char', 'Mousewheel') that needs
// to be forwarded to remote imgui client
//=================================================================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if( hWnd != ghMainWindow )
		return DefWindowProc(hWnd, message, wParam, lParam);

    switch (message)
    {    
    case WM_DESTROY:		PostQuitMessage(0); break;
    case WM_MOUSEWHEEL:		gAppInput.mMouseWheelVertPos	+= static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA); return true;
    case WM_MOUSEHWHEEL:    gAppInput.mMouseWheelHorizPos	+= static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA); return true;
	case WM_COMMAND:
    {
		// Parse the menu selections:
        uint32_t wmId = static_cast<uint32_t>(LOWORD(wParam));
		if( wmId >= kUIClientIFirstID && wmId <= kUIClientILastID )
		{
			UpdateActiveClient(wmId - kUIClientIFirstID);
			return true;
		}
		else if(wmId == IDM_EXIT)
		{
			DestroyWindow(ghMainWindow);
			return true;
		}
    }
    break;
	case WM_SETCURSOR:
	{
		gMouseCursorOwner = LOWORD(lParam) == HTCLIENT;
		if( gMouseCursorOwner )
			return true;	// Eat cursor command to prevent change
		gMouseCursor = nullptr;		
	}
	break;
    case WM_CHAR:
	{
        if (wParam > 0 && wParam < 0x10000 && gAppInput.mKeyCount < sizeof(gAppInput.mKeys)/sizeof(gAppInput.mKeys[0]) )
			gAppInput.mKeys[ gAppInput.mKeyCount++ ] = static_cast<uint16_t>(wParam);
        return true;
	}
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

//=================================================================================================
// INIT CLIENT CONFIG FROM STRING
// Take a commandline string, and create a ClientConfig from it.
// Simple format of (Hostname);(HostPort)
//=================================================================================================
bool InitClientConfigFromString(const char* string, ClientConfig& cmdlineClientOut)
{
	const char* zEntryStart		= string;
	const char* zEntryCur		= string;
	int paramIndex				= 0;
	cmdlineClientOut			= ClientConfig();
	cmdlineClientOut.Transient	= true;
	strcpy_s(cmdlineClientOut.ClientName, "Commandline");

	while( *zEntryCur != 0 )
	{
		zEntryCur++;
		if( (*zEntryCur == ';' || *zEntryCur == 0) )
		{
			if (paramIndex == 0)
				strncpy_s(cmdlineClientOut.HostName, zEntryStart, zEntryCur-zEntryStart);
			else if (paramIndex == 1)
				cmdlineClientOut.HostPort = static_cast<uint32_t>(atoi(zEntryStart));

			cmdlineClientOut.ConnectAuto	= paramIndex >= 1;	//Mark valid for connexion as soon as we have a HostAddress
			zEntryStart						= zEntryCur + 1;
			paramIndex++;
		}
	}
	return cmdlineClientOut.ConnectAuto == true;
}

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
//	other Windows tools, to request a connection between this netImgui server and a netImgui client
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
		ClientConfig cmdlineClient;
		if (InitClientConfigFromString(ComPipeBuffer, cmdlineClient))
			ClientConfig::SetConfig(cmdlineClient);

		CloseHandle(hNamedPipe); // Will recreate the pipe, since it seems only 1 per remote application can exist
		hNamedPipe = INVALID_HANDLE_VALUE;
	}
}

//=================================================================================================
// wWINMAIN
// Main program loop
// Startup, run Main Loop (while active) then Shutdown
//=================================================================================================
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	//------------------------------------------------------------------------------------------------
	// Detect if there's another instance of the netImgui application running
	// (We only want 1 running instance, thus we forward the commandline options and close this one)
	//------------------------------------------------------------------------------------------------
	char commandline[256];
	HANDLE hInterProcessPipe(INVALID_HANDLE_VALUE);
	wcstombs_s(nullptr, commandline, sizeof(commandline), lpCmdLine, sizeof(commandline));
	if( NetImguiNamedPipeMsgSend(commandline) )
		return -1;

	//------------------------------------------------------------------------------------------------
    // Perform application initialization:
	//------------------------------------------------------------------------------------------------
    if (	!Startup (hInstance, nCmdShow) || 
			!NetworkServer::Startup(NetImgui::kDefaultServerPort) ||
			!ServerInfoTab_Startup(NetImgui::kDefaultServerPort) )
    {
		return FALSE;
    }

	//------------------------------------------------------------------------------------------------
	// Parse for auto connect commandline option
	//------------------------------------------------------------------------------------------------	
	ClientConfig::LoadAll();
	ClientConfig cmdlineClient;
	if( InitClientConfigFromString(commandline, cmdlineClient) )
		ClientConfig::SetConfig(cmdlineClient);

	//-------------------------------------------------------------------------
    // Main message loop:
	//-------------------------------------------------------------------------
	MSG msg;
    ZeroMemory(&msg, sizeof(msg));
	auto lastTime = std::chrono::high_resolution_clock::now();	
    while (msg.message != WM_QUIT)
    {
		// Process windows messages 
		if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))		
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
			continue;
        }
		
		// Update UI for new connected or disconnected Clients
		for(uint32_t ClientIdx(0); ClientIdx<ClientRemote::GetCountMax(); ClientIdx++)
		{
			auto& client = ClientRemote::Get(ClientIdx);
			if(client.mbIsConnected && client.mMenuId == 0 && client.mName[0] != 0 )
				AddRemoteClient(ClientIdx);
			if(client.mbIsFree && client.mMenuId != 0 )
				RemoveRemoteClient(ClientIdx);
		}

		// Render the Imgui client currently active
		auto currentTime = std::chrono::high_resolution_clock::now();
		if( std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count() > 4 && gActiveClient != ClientRemote::kInvalidClient)
		{			
			auto& activeClient = ClientRemote::Get(gActiveClient);
			// Redraw the ImGui Server tab with infos.
			// This tab works exactly like a remote client connected to this server, 
			// to allow debuging of communications
			ServerInfoTab_Draw();

			// Gather keyboard/mouse status and collected infos from window message loop, 
			// and send theses Input to active Client (selected tab).
			// Note: Client will only emit DrawData when they receive this input command first
			activeClient.UpdateInputToSend(ghMainWindow, gAppInput);

			// Render the last received ImGui DrawData from active client
			auto pDrawCmd = activeClient.GetDrawFrame();
			dx::Render(activeClient.mvTextures, pDrawCmd);
			
			// Update the mouse cursor to reflect the one ImGui expect			
			if( pDrawCmd )
			{
				LPTSTR wantedCursor = IDC_ARROW;
				switch (pDrawCmd->mMouseCursor)
				{
				case ImGuiMouseCursor_Arrow:        wantedCursor = IDC_ARROW; break;
				case ImGuiMouseCursor_TextInput:    wantedCursor = IDC_IBEAM; break;
				case ImGuiMouseCursor_ResizeAll:    wantedCursor = IDC_SIZEALL; break;
				case ImGuiMouseCursor_ResizeEW:     wantedCursor = IDC_SIZEWE; break;
				case ImGuiMouseCursor_ResizeNS:     wantedCursor = IDC_SIZENS; break;
				case ImGuiMouseCursor_ResizeNESW:   wantedCursor = IDC_SIZENESW; break;
				case ImGuiMouseCursor_ResizeNWSE:   wantedCursor = IDC_SIZENWSE; break;
				case ImGuiMouseCursor_Hand:         wantedCursor = IDC_HAND; break;
				case ImGuiMouseCursor_NotAllowed:   wantedCursor = IDC_NO; break;
				default:							wantedCursor = IDC_ARROW; break;
				}
		
				if( gMouseCursorOwner && gMouseCursor != wantedCursor )
				{
					gMouseCursor = wantedCursor;
					::SetCursor(::LoadCursor(nullptr, gMouseCursor));
				}
			}						
			
			lastTime = currentTime;
			std::this_thread::sleep_for(std::chrono::milliseconds(8));
		}

		// Receive request from other application, for new client config
		NetImguiNamedPipeMsgReceive(hInterProcessPipe);
    }

	//-------------------------------------------------------------------------
	// Release all resources
	//-------------------------------------------------------------------------
	ServerInfoTab_Shutdown();
	NetworkServer::Shutdown();
	dx::Shutdown();
	ClientRemote::Shutdown();
	ClientConfig::Clear();
	CloseHandle(hInterProcessPipe);
    return static_cast<int>(msg.wParam);
}
