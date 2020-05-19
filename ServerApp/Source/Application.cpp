//=================================================================================================
// Application.cpp
//
// Entry point of this application. 
// Initialize the app & window, and run the mainloop with messages processing
//=================================================================================================
#include "stdafx.h"

#include "Private/NetImgui_WarningDisableStd.h"
#include <array>
#include <chrono>
#include "Private/NetImgui_WarningReenable.h"

#include "../resource.h"
#include "../DirectX/DirectX11.h"
#include "ServerNetworking.h"
#include "ServerInfoTab.h"
#include "RemoteClient.h"

constexpr uint32_t	kLoadStringMax		= 100;
constexpr uint32_t	kClientCountMax		= 8;
constexpr uint32_t	kUIClientIFirstID	= 1000;
constexpr uint32_t	kUIClientILastID	= (kUIClientIFirstID + kClientCountMax - 1);
constexpr uint32_t	kInvalidClient		= static_cast<uint32_t>(-1);

// Global Variables:
static HINSTANCE									ghApplication;							// Current instance
static HWND											ghMainWindow;							// Main Windows
static WCHAR										szTitle[kLoadStringMax];				// The title bar text
static WCHAR										szWindowClass[kLoadStringMax];			// the main window class name
static InputUpdate									gAppInput;								// Input capture in Window message loop and waiting to be sent to remote client
static uint32_t										gActiveClient		= kInvalidClient;	// Currently selected remote Client for input & display
static LPTSTR										gMouseCursor		= nullptr;			// Last mouse cursor assigned by received drawing command
static bool											gMouseCursorOwner	= false;			// Keep track if cursor is over Client drawing region, and thus should have its cursor updated
static std::array<ClientRemote, kClientCountMax>*	gpClients			= nullptr;			// Table of all potentially connected clients to this server

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void UpdateActiveClient(uint32_t NewClientId)
{
	if( NewClientId != gActiveClient )
	{
		char zName[96];
		if( gActiveClient != kInvalidClient)
		{
			ClientRemote& client = (*gpClients)[gActiveClient];
			sprintf_s(zName, "    %s    ", client.mName);
			ModifyMenuA(GetMenu(ghMainWindow), static_cast<UINT>(client.mMenuId), MF_BYCOMMAND|MF_ENABLED, client.mMenuId, zName);
			client.mbIsActive = false;
		}
		
		gActiveClient = NewClientId;
		if( gActiveClient != kInvalidClient)
		{
			ClientRemote& client = (*gpClients)[gActiveClient];
			sprintf_s(zName, "--[ %s ]--", client.mName);
			ModifyMenuA(GetMenu(ghMainWindow), static_cast<UINT>(client.mMenuId), MF_BYCOMMAND|MF_GRAYED, client.mMenuId, zName);
			client.mbIsActive = true;
		}		
		DrawMenuBar(ghMainWindow);
	}
}

void AddRemoteClient(uint32_t NewClientIndex)
{	
	(*gpClients)[NewClientIndex].mMenuId = kUIClientIFirstID + NewClientIndex;
	AppendMenuA(GetMenu(ghMainWindow), MF_STRING, (*gpClients)[NewClientIndex].mMenuId, "");
	UpdateActiveClient(NewClientIndex);
}

void RemoveRemoteClient(uint32_t OldClientIndex)
{
	if((*gpClients)[OldClientIndex].mMenuId != 0 ) // Client 0 is the ServerTab and cannot be removed
	{
		RemoveMenu(GetMenu(ghMainWindow), static_cast<UINT>((*gpClients)[OldClientIndex].mMenuId), MF_BYCOMMAND);
		(*gpClients)[OldClientIndex].Reset();
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
	gpClients			= new std::array<ClientRemote, kClientCountMax>();
		
	if (!ghMainWindow)
		return false;

	if( !dx::Startup(ghMainWindow) )
		return false;

	memset(&gAppInput, 0, sizeof(gAppInput));
	ShowWindow(ghMainWindow, nCmdShow);
	UpdateWindow(ghMainWindow);
	return true;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return static_cast<INT_PTR>(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return static_cast<INT_PTR>(TRUE);
        }
        break;
    }
    return static_cast<INT_PTR>(FALSE);
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
		}
		else
		{
			switch (wmId)
			{
			case IDM_ABOUT:	DialogBox(ghApplication, MAKEINTRESOURCE(IDD_ABOUTBOX), ghMainWindow, About);	break;
			case IDM_EXIT:	DestroyWindow(ghMainWindow); break;
			default: return DefWindowProc(ghMainWindow, message, wParam, lParam);
			}
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
// wWINMAIN
// Main program loop
// Startup, run Main Loop (while active) then Shutdown
//=================================================================================================
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	//-------------------------------------------------------------------------
    // Perform application initialization:
	//-------------------------------------------------------------------------
    if (	!Startup (hInstance, nCmdShow) || 
			!NetworkServer::Startup(&(*gpClients)[0], static_cast<uint32_t>(gpClients->size()), NetImgui::kDefaultServerPort ) ||
			!ServerInfoTab_Startup(NetImgui::kDefaultServerPort) )
    {
		return FALSE;
    }

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
		uint32_t clientConnectedCount(0);
		for(uint32_t ClientIdx(0); ClientIdx<static_cast<uint32_t>((*gpClients).size()); ClientIdx++)
		{
			auto& client = (*gpClients)[ClientIdx];
			if(client.mbConnected == true && client.mMenuId == 0 && client.mName[0] != 0 )
				AddRemoteClient(ClientIdx);
			if(client.mbConnected == false && client.mMenuId != 0 )
				RemoveRemoteClient(ClientIdx);
			clientConnectedCount += client.mbConnected ? 1 : 0;
		}

		// Render the Imgui client currently active
		auto currentTime = std::chrono::high_resolution_clock::now();
		if( std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count() > 4 )
		{
			
			// Redraw the ImGui Server tab with infos.
			// This tab works exactly like a remote client connected to this server, 
			// to allow debuging of communications
			ServerInfoTab_Draw(clientConnectedCount);

			// Gather keyboard/mouse status and collected infos from window message loop, 
			// and send theses Input to active Client (selected tab).
			// Note: Client will only emit DrawData when they receive this input command first
			(*gpClients)[gActiveClient].UpdateInputToSend(ghMainWindow, gAppInput);

			// Render the last received ImGui DrawData from active client
			auto pDrawCmd = (*gpClients)[gActiveClient].GetDrawFrame();
			dx::Render((*gpClients)[gActiveClient].mvTextures, pDrawCmd);
			
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
		}
		
    }

	//-------------------------------------------------------------------------
	// Release all resources
	//-------------------------------------------------------------------------
	ServerInfoTab_Shutdown();
	NetworkServer::Shutdown();
	dx::Shutdown();
	delete gpClients;

    return static_cast<int>(msg.wParam);
}
