#include <stdio.h>
#include <algorithm>
#include <NetImgui_Api.h>
#include <Private/NetImgui_CmdPackets.h>

#include "NetImguiServer_App.h"
#include "NetImguiServer_UI.h"
#include "NetImguiServer_RemoteClient.h"
#include "NetImguiServer_Config.h"
#include "NetImguiServer_Network.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../ThirdParty/stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

namespace NetImguiServer { namespace UI
{

constexpr uint32_t	kClientRemoteInvalid				= 0xFFFFFFFF;
constexpr char		kNetImguiURL[]						= "https://github.com/sammyfreg/netImgui";
const char*			kDataSizeUnits[]					= {"B", "KB", "MB", "GB"};
static const ImVec4	kColorTitle							= ImVec4(0.7f,1.0f,0.7f,1.f);
static const ImVec4	kColorContent						= ImVec4(0.7f,0.75f,0.7f,1.f);
static ImGuiID		gMainDockID							= 0;
static void*		gpHAL_BackgroundTexture				= nullptr;
static ImVec2		gvBackgroundTextureSize				= ImVec2(0,0);
static ImVec4		gBackgroundColor					= ImVec4(0,0,0,0);

static uint32_t		gPopup_ConfirmDisconnect_ClientIdx	= kClientRemoteInvalid;
static uint32_t		gPopup_ConfirmDelete_ConfigIdx		= NetImguiServer::Config::Client::kInvalidRuntimeID;
static bool			gPopup_AboutNetImgui_Show			= false;
static bool			gPopup_ServerConfig_Show			= false;
static NetImguiServer::Config::Client*	gPopup_ClientConfig_pConfig	= nullptr;

//=================================================================================================
// Convert a memory size to a displayable value
//=================================================================================================
uint8_t ConvertDataAmount(uint64_t& dataSize)
{
	uint8_t outUnitIdx	= 0;
	for(size_t i(0); i<IM_ARRAYSIZE(kDataSizeUnits); ++i)
	{
		outUnitIdx	+= dataSize >= 1024 * 100 ? 1 : 0;
		dataSize	/= outUnitIdx > i ? 1024 : 1;
	}
	return outUnitIdx;
}

//=================================================================================================
// Fill current window with our main background picture
//=================================================================================================
void DrawBackground( ImVec4 Tint=ImVec4(1.f,1.f,1.f,1.f) )
{
	const ImVec2 savedPos	= ImGui::GetCursorPos();
	const ImVec2 areaSize	= ImGui::GetContentRegionAvail();
	const float ratioH		= gvBackgroundTextureSize.x / areaSize.x;
	const float ratioV		= gvBackgroundTextureSize.y / areaSize.y;
	float bgSizeX			= ratioH > ratioV ? areaSize.x : areaSize.y * gvBackgroundTextureSize.x / gvBackgroundTextureSize.y;
	float bgSizeY			= ratioH < ratioV ? areaSize.y : areaSize.x * gvBackgroundTextureSize.y / gvBackgroundTextureSize.x;
	float uvOffsetX			= (areaSize.x - bgSizeX) / 2.f;
	float uvOffsetY			= (areaSize.y - bgSizeY) / 2.f;
	ImGui::SetCursorPos(ImVec2(savedPos.x+uvOffsetX, savedPos.y+uvOffsetY));
	ImGui::Image(reinterpret_cast<ImTextureID>(gpHAL_BackgroundTexture), ImVec2(bgSizeX, bgSizeY), ImVec2(0, 0), ImVec2(1, 1), Tint);
	ImGui::SetCursorPos(savedPos);
}

//=================================================================================================
// If last Imgui item is hovered, display info on the provided remote client
//=================================================================================================
void ClientInfoTooltip(const RemoteClient::Client& Client)
{
	if (ImGui::IsItemHovered())
	{		
		NetImguiServer::Config::Client config;
		constexpr float width(60.f);
		
		auto elapsedTime	= std::chrono::steady_clock::now() - Client.mConnectedTime;
		int tmSec			= static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count() % 60);
		int tmMin			= static_cast<int>(std::chrono::duration_cast<std::chrono::minutes>(elapsedTime).count() % 60);
		int tmHour			= static_cast<int>(std::chrono::duration_cast<std::chrono::hours>(elapsedTime).count());
		bool validConfig	= NetImguiServer::Config::Client::GetConfigByID(Client.mClientConfigID, config);
		
		uint64_t rxData(Client.mStatsDataRcvd[0]);
		uint64_t txData(Client.mStatsDataSent[0]);
		uint8_t txUnitIdx = ConvertDataAmount(txData);
		uint8_t rxUnitIdx = ConvertDataAmount(rxData);

		ImGui::BeginTooltip();
		ImGui::TextUnformatted("Name");		ImGui::SameLine(width); ImGui::TextColored(kColorContent, ": %s", Client.mInfoName);
		ImGui::TextUnformatted("Config");	ImGui::SameLine(width); ImGui::TextColored(kColorContent, ": %s", validConfig ? config.mClientName : "None");
		ImGui::TextUnformatted("Host");		ImGui::SameLine(width); ImGui::TextColored(kColorContent, ": %s", Client.mConnectHost);
		ImGui::TextUnformatted("Port");		ImGui::SameLine(width); ImGui::TextColored(kColorContent, ": %i", Client.mConnectPort);
		ImGui::TextUnformatted("ImGui");	ImGui::SameLine(width); ImGui::TextColored(kColorContent, ": %s", Client.mInfoImguiVerName);
		ImGui::TextUnformatted("Time");		ImGui::SameLine(width); ImGui::TextColored(kColorContent, ": %03ih%02i:%02i", tmHour,tmMin,tmSec );
		ImGui::TextUnformatted("Fps");		ImGui::SameLine(width); ImGui::TextColored(kColorContent, ": %04.1f", Client.mStatsFPS );
		ImGui::TextUnformatted("Data");		ImGui::SameLine(width); ImGui::TextColored(kColorContent, ": (Rx) %7i KB/s \t(Tx) %7i KB/s", Client.mStatsRcvdBps/1024, Client.mStatsSentBps/1024);
		ImGui::NewLine();					ImGui::SameLine(width); ImGui::TextColored(kColorContent, ": (Rx) %7i %s   \t(Tx) %7i %s", static_cast<int>(rxData), kDataSizeUnits[rxUnitIdx], static_cast<int>(txData), kDataSizeUnits[txUnitIdx]);
		ImGui::EndTooltip();
	}
}

//=================================================================================================
// When a Remote Client request is made, first makes sure to confirm with user
//=================================================================================================
void Popup_ConfirmDisconnect()
{	
	bool pendingDisconnectOpen(gPopup_ConfirmDisconnect_ClientIdx != kClientRemoteInvalid);
	if (pendingDisconnectOpen)
	{
		
		RemoteClient::Client& client			= RemoteClient::Client::Get(gPopup_ConfirmDisconnect_ClientIdx);
		bool wantExit							= ImGui::IsKeyPressed(static_cast<int>(NetImgui::Internal::CmdInput::eVirtualKeys::vkKeyboardEscape), false);
		ImGuiWindowClass windowClass;
		windowClass.ViewportFlagsOverrideSet	= ImGuiViewportFlags_TopMost;		
		ImGui::SetNextWindowClass(&windowClass);
		ImGui::OpenPopup("Confirmation##DEL");
		if (ImGui::BeginPopupModal("Confirmation##DEL", &pendingDisconnectOpen, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::NewLine();
			ImGui::TextUnformatted("Are you certain you want to disconnect\nfrom this client configuration ?");
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::ListBoxHeader("##", 1, 2))
			{				
				ImGui::TextUnformatted(client.mInfoName);
				ImGui::ListBoxFooter();
			}

			ImGui::NewLine();
			ImGui::Separator();			
			if (ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvailWidth() / 2.f, 0)) || wantExit ) 
			{
				pendingDisconnectOpen		= false;
			}
			ImGui::SetItemDefaultFocus();

			ImGui::SameLine();
			if (ImGui::Button("Yes", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)) && gPopup_ConfirmDisconnect_ClientIdx != kClientRemoteInvalid)
			{
				client.mbPendingDisconnect	= true;
				pendingDisconnectOpen		= false;
							
			}
			ImGui::EndPopup();
		}		
	}

	if( !pendingDisconnectOpen ){
		gPopup_ConfirmDisconnect_ClientIdx	= kClientRemoteInvalid;
	}
}

//=================================================================================================
// Display information about NetImgui Server application
//=================================================================================================
void Popup_AboutNetImgui()
{
	static bool sbURLHover = false;
	if( gPopup_AboutNetImgui_Show )
	{		
		ImGuiWindowClass windowClass;
		windowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_TopMost;
		ImGui::SetNextWindowClass(&windowClass);
		ImGui::OpenPopup("About NetImgui...");
		if (ImGui::BeginPopupModal("About NetImgui...", &gPopup_AboutNetImgui_Show, ImGuiWindowFlags_AlwaysAutoResize))
		{			
			ImGui::NewLine();
			ImGui::Text("Version : %s", NETIMGUI_VERSION);
			ImGui::NewLine();
			ImGui::Text("For more informations : ");
			
			ImGui::TextColored(sbURLHover ? ImColor(0.8f, 0.8f, 1.f,1.f) : ImColor(0.5f, 0.5f, 1.f,1.f), kNetImguiURL);
			sbURLHover = ImGui::IsItemHovered();
			if (sbURLHover && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				NetImguiServer::App::HAL_ShellCommand(kNetImguiURL);
			}

			ImGui::NewLine();
			ImGui::TextUnformatted("Note: Commandline can be used to connect to a Client."); 
			ImGui::TextColored(kColorContent, "Syntax : (HostName);(HostPort)");	
			ImGui::TextColored(kColorContent, "Example: netImgui_Server.exe 127.0.0.1;8889");
			ImGui::NewLine();
			ImGui::Separator();

			bool wantExit	 = ImGui::IsKeyPressed(static_cast<int>(NetImgui::Internal::CmdInput::eVirtualKeys::vkKeyboardEscape), false);
			wantExit		|= ImGui::IsKeyPressed(static_cast<int>(NetImgui::Internal::CmdInput::eVirtualKeys::vkKeyboardEnter), false);
			if( ImGui::Button("Close", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)) || wantExit) 
				gPopup_AboutNetImgui_Show = false;
			ImGui::EndPopup();
		}
	}
}

//=================================================================================================
// Edit the Server configuration
//=================================================================================================
void Popup_ServerConfig()
{
	static int sEditPort					= -1;
	static float sEditRefreshFPSActive		= 0;
	static float sEditRefreshFPSInactive	= 0;

	if( gPopup_ServerConfig_Show )
	{		
		if( sEditPort == -1 ){
			sEditPort				= static_cast<int>(NetImguiServer::Config::Server::sPort);
			sEditRefreshFPSActive	= NetImguiServer::Config::Server::sRefreshFPSActive;
			sEditRefreshFPSInactive	= NetImguiServer::Config::Server::sRefreshFPSInactive;
		}
		ImGuiWindowClass windowClass;
		windowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_TopMost;
		ImGui::SetNextWindowClass(&windowClass);
		ImGui::OpenPopup("Server Configuration");
		if (ImGui::BeginPopupModal("Server Configuration", &gPopup_ServerConfig_Show, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::NewLine();

			// --- Port ---						
			ImGui::TextUnformatted("Port waiting for connection requests");
			if (ImGui::InputInt("Port", &sEditPort, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
				sEditPort = std::min<int>(0xFFFF, std::max<int>(1, sEditPort));
			}
			ImGui::SameLine();
			if (ImGui::Button("Default")) {
				sEditPort = NetImgui::kDefaultServerPort;
			}

			// --- Refresh ---	
			ImGui::SliderFloat("Active Window", &sEditRefreshFPSActive, 0.f, 60.f, "%2.f Fps" );
			if( ImGui::IsItemHovered() ){
				ImGui::SetTooltip("How often we refresh content of *visible* and *focused* clients.\nNote: Lowering this will reduce network traffic.");
			}

			ImGui::SliderFloat("Inactive Window", &sEditRefreshFPSInactive, 0.f, 60.f, "%2.f Fps" );
			if( ImGui::IsItemHovered() ){
				ImGui::SetTooltip("How often we refresh content of *visible* and *unfocused* clients.\nNote: Lowering this will reduce network traffic.");
			}

			// --- Save/Cancel ---
			ImGui::NewLine();
			ImGui::Separator();
			bool wantExit				= ImGui::IsKeyPressed(static_cast<int>(NetImgui::Internal::CmdInput::eVirtualKeys::vkKeyboardEscape), false);
			gPopup_ServerConfig_Show	&= !ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvailWidth() / 2.f, 0)) && !wantExit;
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Save", ImVec2(ImGui::GetContentRegionAvailWidth(), 0))) {
				NetImguiServer::Config::Server::sPort				= static_cast<uint32_t>(sEditPort);
				NetImguiServer::Config::Server::sRefreshFPSActive	= sEditRefreshFPSActive;
				NetImguiServer::Config::Server::sRefreshFPSInactive	= sEditRefreshFPSInactive;
				NetImguiServer::Config::Client::SaveAll();
				gPopup_ServerConfig_Show = false;
			}
			ImGui::EndPopup();
		}
	}

	if( !gPopup_ServerConfig_Show )
	{
		sEditPort = -1;
	}
}

//=================================================================================================
// Edit a new or existing client settings
//=================================================================================================
void Popup_ClientConfigEdit()
{	
	bool bOpenEdit(gPopup_ClientConfig_pConfig != nullptr);
	if (bOpenEdit)
	{		
		
		ImGuiWindowClass windowClass;
		windowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_TopMost;
		ImGui::SetNextWindowClass(&windowClass);
		ImGui::OpenPopup("Edit Client Info");
		if (ImGui::BeginPopupModal("Edit Client Info", &bOpenEdit, ImGuiWindowFlags_AlwaysAutoResize))
		{
			
			ImGui::NewLine();
			// --- Name ---
			ImGui::InputText("Display Name", gPopup_ClientConfig_pConfig->mClientName, sizeof(gPopup_ClientConfig_pConfig->mClientName));
			
			// --- IP Address ---
			ImGui::InputText("Host Name", gPopup_ClientConfig_pConfig->mHostName, sizeof(gPopup_ClientConfig_pConfig->mHostName));
			if( ImGui::IsItemHovered() ){
				ImGui::SetTooltip("IP address or name of the host to reach");
			}

			// --- Port ---
			int port = static_cast<int>(gPopup_ClientConfig_pConfig->mHostPort);
			if( ImGui::InputInt("Host Port", &port, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue) ){
				gPopup_ClientConfig_pConfig->mHostPort = std::min<int>(0xFFFF, std::max<int>(1, port));
			}
			ImGui::SameLine();
			if (ImGui::Button("Default")){
				gPopup_ClientConfig_pConfig->mHostPort = NetImgui::kDefaultClientPort;
			}

			// --- Auto ---
			ImGui::Checkbox("Auto Connect", &gPopup_ClientConfig_pConfig->mConnectAuto);

			// --- Save/Cancel ---
			ImGui::NewLine();
			ImGui::Separator();
			bOpenEdit &= !ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvailWidth()/2.f, 0));
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Save", ImVec2(ImGui::GetContentRegionAvailWidth(), 0))){
				NetImguiServer::Config::Client::SetConfig(*gPopup_ClientConfig_pConfig);
				NetImguiServer::Config::Client::SaveAll();
				bOpenEdit = false;
			}
			ImGui::EndPopup();
		}

		bool wantExit = ImGui::IsKeyPressed(static_cast<int>(NetImgui::Internal::CmdInput::eVirtualKeys::vkKeyboardEscape), false);
		bOpenEdit &= !wantExit;
	}	

	if( !bOpenEdit ){
		NetImgui::Internal::netImguiDeleteSafe(gPopup_ClientConfig_pConfig);
	}
}


//=================================================================================================
// Client Config Delete Confirmation
//=================================================================================================
void Popup_ClientConfigDelete()
{
	NetImguiServer::Config::Client config;
	bool bOpenDelConfirm(NetImguiServer::Config::Client::GetConfigByID(gPopup_ConfirmDelete_ConfigIdx, config));
	if( bOpenDelConfirm )
	{
		ImGuiWindowClass windowClass;
		windowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_TopMost;
		ImGui::SetNextWindowClass(&windowClass);
		ImGui::OpenPopup("Confirmation##DEL");
		if (ImGui::BeginPopupModal("Confirmation##DEL", &bOpenDelConfirm, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::NewLine();
			ImGui::TextUnformatted("Are you certain you want to remove\nthis client configuration ?");
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::ListBoxHeader("##", 1, 2))
			{				
				ImGui::TextUnformatted(config.mClientName);
				ImGui::ListBoxFooter();
			}

			ImGui::NewLine();
			ImGui::Separator();
			bOpenDelConfirm &= !ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvailWidth()/2.f, 0));
			ImGui::SetItemDefaultFocus();

			ImGui::SameLine();
			if (ImGui::Button("Yes", ImVec2(ImGui::GetContentRegionAvailWidth(), 0))){
				NetImguiServer::Config::Client::DelConfig(gPopup_ConfirmDelete_ConfigIdx);
				NetImguiServer::Config::Client::SaveAll();
				bOpenDelConfirm = false;
			}
			ImGui::EndPopup();
		}

		bool wantExit	= ImGui::IsKeyPressed(static_cast<int>(NetImgui::Internal::CmdInput::eVirtualKeys::vkKeyboardEscape), false);
		bOpenDelConfirm &= !wantExit;
		if( !bOpenDelConfirm ){
			gPopup_ConfirmDelete_ConfigIdx = NetImguiServer::Config::Client::kInvalidRuntimeID;
		}
	}
}

//=================================================================================================
// Setuping the docking of our application
//=================================================================================================
void DrawImguiContent_SetupDocking(const ImVec4& BackgroundTint)
{
	if (gMainDockID == 0 )
	{
		gMainDockID = ImGui::GetID("MainDockID");
	}

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiViewport* viewport         = ImGui::GetMainViewport();	
    ImGuiWindowFlags window_flags   = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;
    window_flags                    |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags                    |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGui::SetNextWindowPos(viewport->GetWorkPos());
    ImGui::SetNextWindowSize(viewport->GetWorkSize());
    ImGui::SetNextWindowViewport(viewport->ID);
	
	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::Begin("DockSpace", nullptr, window_flags);
	ImGui::PopStyleVar(2);
	DrawBackground(BackgroundTint);
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGui::DockSpace(gMainDockID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	}
	
    ImGui::End();
}

//=================================================================================================
// Display the dockable window of each connected client we can interact with
//=================================================================================================
void DrawImguiContent_Clients()
{	
	ImGui::PushStyleColor(ImGuiCol_WindowBg, gBackgroundColor);	// When remote client window is floating
	ImGui::PushStyleColor(ImGuiCol_ChildBg, gBackgroundColor);	// When Remote client window is docked
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);	
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	
	//---------------------------------------------------------------------------------------------	
	// Display each connected client window
	//---------------------------------------------------------------------------------------------
	bool hasConnection(false);
	for(uint32_t i(0); i<RemoteClient::Client::GetCountMax(); ++i)
	{
		RemoteClient::Client& client = RemoteClient::Client::Get(i);
		if( client.mbIsConnected )
		{			
			ImGui::PushID(i);			
			ImGui::SetNextWindowBgAlpha(1.0);
			ImGui::SetNextWindowDockID(gMainDockID, ImGuiCond_Once);
			bool bOpened(true);
			hasConnection = true;
			client.mbIsVisible = ImGui::Begin(client.mWindowID, &bOpened, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
			ClientInfoTooltip(client);
			ImGui::PopStyleVar(1);
			if( client.mbIsVisible )
			{
				// Capture input to forward to remote client, and update drawing area size
				ImVec2 areaSize		= ImGui::GetContentRegionAvail();
				client.mAreaSizeX	= static_cast<uint16_t>(std::max<float>(8.f,areaSize.x)); //Prevents issue with render target of size 0
				client.mAreaSizeY	= static_cast<uint16_t>(std::max<float>(8.f,areaSize.y));
				client.CaptureImguiInput();

				// Display remote client drawing results
				if (client.mpHAL_AreaTexture && areaSize.x > 0 && areaSize.y > 0) 
				{					
					// Add fake button to discard mouse input (prevent window moving when draging inside client area)
					ImVec2 savedPos			= ImGui::GetCursorPos();
					const ImVec4 tint_col	= ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					const ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);					
					ImGui::InvisibleButton("canvas", areaSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
					ImGui::SetCursorPos(savedPos);

					// Display Client Context
					DrawBackground(ImVec4(1.f, 1.f, 1.f, 0.15f));
					ImGui::Image(reinterpret_cast<ImTextureID>(client.mpHAL_AreaTexture), areaSize, ImVec2(0, 0), ImVec2(1, 1), tint_col, border_col);
					if( ImGui::IsItemHovered() ){
						ImGui::SetMouseCursor(client.mMouseCursor);
					}
				}
			}
			ImGui::End();
			if(!bOpened && !client.mbPendingDisconnect )
			{
				gPopup_ConfirmDisconnect_ClientIdx = i;
			}
			ImGui::PopID();			
		}
	}
	
	//---------------------------------------------------------------------------------------------
	// Display some instruction when no connection detected
	//---------------------------------------------------------------------------------------------
	if (!hasConnection)
	{
		ImGui::SetNextWindowBgAlpha(1.0);
		ImGui::SetNextWindowDockID(gMainDockID, ImGuiCond_Always);
		if (ImGui::Begin("Information", nullptr, 0))
		{
			DrawBackground(ImVec4(1.f, 1.f, 1.f, 0.15f));

			ImGui::TextColored(kColorTitle, "%s", "Purpose:");
			ImGui::PushStyleColor(ImGuiCol_Text, kColorContent);
			ImGui::TextWrapped("This 'NetImgui Server' application allows connection to clients running with the 'NetImGui Library.");
			ImGui::PopStyleColor();
			ImGui::NewLine();

			ImGui::TextColored(kColorTitle, "%s", "Instructions:");
			ImGui::PushStyleColor(ImGuiCol_Text, kColorContent);
			ImGui::TextWrapped("There are 2 ways of establishing a connection between this Server and a Client.");
			ImGui::TextWrapped(" (A) The client can connect directly to this server, using 'NetImgui::ConnectToApp(...)' on port %i", NetImguiServer::Config::Server::sPort);
			ImGui::TextWrapped(" (B) The client can wait for a Server connection using 'NetImgui::ConnectFrom(...)' and adding the Client configuration on the Server.");
			ImGui::PopStyleColor();
			ImGui::NewLine();

			ImGui::TextColored(kColorTitle, "%s", "Note:");
			ImGui::PushStyleColor(ImGuiCol_Text, kColorContent);
			ImGui::TextWrapped("'Multiple clients can be connected to this server. Each client window can be undocked and moved around independently.");
			ImGui::PopStyleColor();			
		}
		ImGui::End();
	}

	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(2);
}

//=================================================================================================
// Main Menu client table entry detail (1 line in clients table)
//=================================================================================================
void DrawImguiContent_MainMenu_Clients_Entry(RemoteClient::Client* pClient, NetImguiServer::Config::Client* pClientConfig)
{
	const bool hasValidConfig = pClientConfig && !pClientConfig->mTransient;

	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
		
	// Name / Status
	ImGui::PushStyleColor(ImGuiCol_CheckMark, pClient && pClient->mbIsConnected ? ImVec4(0.7f, 1.f, 0.25f, 1.f) : ImVec4(1.f, 0.35f, 0.35f, 1.f));
	ImGui::RadioButton("##Connected", true);
	ImGui::PopStyleColor();	
	if( pClient && pClient->mbIsConnected  ){
		bool selected(false);
		ImGui::SameLine();
		if (ImGui::Selectable(pClient->mInfoName, &selected, ImGuiSelectableFlags_DontClosePopups/* | ImGuiSelectableFlags_SpanAllColumns*/)) {
			ImGui::SetWindowCollapsed(pClient->mWindowID, false);
			ImGui::SetWindowFocus(pClient->mWindowID);
		}
		ClientInfoTooltip(*pClient);
	}
	
	ImGui::SameLine(); ImGui::Text("(%s)", pClientConfig ? pClientConfig->mClientName : "No Config");
	ImGui::TableNextColumn();

	// Hostname info IP/Port
	ImGui::Text("%s : %i", pClientConfig ? pClientConfig->mHostName : pClient->mConnectHost, pClientConfig ? pClientConfig->mHostPort : pClient->mConnectPort);
	if( ImGui::IsItemHovered() && pClient ){
		ImGui::SetTooltip("Port assigned for communications:%i", pClient->mConnectPort );
	}
	ImGui::TableNextColumn();
	
	// Config: AutoConnect
	if( hasValidConfig && ImGui::Checkbox("##auto", &pClientConfig->mConnectAuto)) {
		NetImguiServer::Config::Client::SetProperty_ConnectAuto(pClientConfig->mRuntimeID, pClientConfig->mConnectAuto);
		NetImguiServer::Config::Client::SaveAll();
	}
	if( ImGui::IsItemHovered() ){
		ImGui::SetTooltip("Toggle auto connection attempt on this client");
	}
	ImGui::TableNextColumn();

	if( hasValidConfig )
	{
		if (ImGui::Button("-") )
		{		
			gPopup_ConfirmDelete_ConfigIdx = pClientConfig->mRuntimeID;
		}		
		ImGui::SameLine(); 
		if ( ImGui::Button("...") && !gPopup_ClientConfig_pConfig ) // Only 1 config edit at a time, preventing mem leak
		{
			gPopup_ClientConfig_pConfig	= NetImgui::Internal::netImguiNew<NetImguiServer::Config::Client>(*pClientConfig);
		}		
	}
	ImGui::TableNextColumn();

	// Config: Connection
	if( pClient && !pClient->mbPendingDisconnect && ImGui::Button("Disconnect", ImVec2(80,0 )) )
	{
		gPopup_ConfirmDisconnect_ClientIdx = pClient->mClientIndex;
	}
	else if( pClientConfig )
	{
		if( pClientConfig->mTransient ){
			ImGui::TextUnformatted("(Request)");
		}
		else if (!pClientConfig->mConnected && !pClientConfig->mConnectRequest && ImGui::Button("Connect", ImVec2(80,0 )) ){
			NetImguiServer::Config::Client::SetProperty_ConnectRequest(pClientConfig->mRuntimeID, true);
		}
	}
}

//=================================================================================================
// MainMenu Entry : Client 
// Display the list of connected and unconnected clients
//=================================================================================================
void DrawImguiContent_MainMenu_Clients()
{
	int connectedClient(0);
	constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4,4));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(4.f, 4.f));
	if (ImGui::BeginTable("##TableClient", 5, flags))
	{
		ImGui::TableSetupColumn("Name",				ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("HostName (IP)",	ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Auto",				ImGuiTableColumnFlags_WidthAuto);
		ImGui::TableSetupColumn("###Config",		ImGuiTableColumnFlags_WidthAuto|ImGuiTableColumnFlags_NoResize|ImGuiTableColumnFlags_NoReorder);
		ImGui::TableSetupColumn("###Connection",	ImGuiTableColumnFlags_WidthAuto|ImGuiTableColumnFlags_NoResize|ImGuiTableColumnFlags_NoReorder);
		ImGui::TableHeadersRow();

		// First, display all connected clients without a config
		ImGui::PushID("Connected");
		for (uint32_t i(0); i < RemoteClient::Client::GetCountMax(); ++i)
		{
			RemoteClient::Client& client = RemoteClient::Client::Get(i);
			if( client.mbIsConnected && client.mClientConfigID == NetImguiServer::Config::Client::kInvalidRuntimeID ){
				ImGui::PushID(i);
				DrawImguiContent_MainMenu_Clients_Entry(&client, nullptr);
				ImGui::PopID();
				++connectedClient;
			}
		}
		ImGui::PopID();

		// Next, display all connected clients with a config
		NetImguiServer::Config::Client clientConfig;
		ImGui::PushID("ConfigConnected");
		for (uint32_t i(0); i < RemoteClient::Client::GetCountMax(); ++i)
		{
			RemoteClient::Client& client = RemoteClient::Client::Get(i);
			if( client.mbIsConnected && NetImguiServer::Config::Client::GetConfigByID(client.mClientConfigID, clientConfig) ){
				ImGui::PushID(i);
				DrawImguiContent_MainMenu_Clients_Entry(&client, &clientConfig);
				ImGui::PopID();
				++connectedClient;
			}
		}
		ImGui::PopID();

		// Finally, display unconnected client configs
		ImGui::PushID("ConfigUnconnected");
		for (int i = 0; NetImguiServer::Config::Client::GetConfigByIndex(i, clientConfig); ++i)
		{
			if( !clientConfig.mConnected ){
				ImGui::PushID(i);
				DrawImguiContent_MainMenu_Clients_Entry(nullptr, &clientConfig);
				ImGui::PopID();
			}
		}
		ImGui::PopID();

		// Info about Server waiting on connection
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.f, 0.35f, 0.35f, 1.f));
			ImGui::RadioButton("##ServerPortStatus", !NetImguiServer::Network::IsWaitingForConnection());
			ImGui::PopStyleColor();	
			ImGui::SameLine();
			if( NetImguiServer::Network::IsWaitingForConnection() ){
				ImGui::Text("Waiting for clients on port: %i", static_cast<int>(NetImguiServer::Config::Server::sPort));
			}
			else{
				ImGui::Text("Unable to accept clients on port: %i", static_cast<int>(NetImguiServer::Config::Server::sPort));
			}
		}
		ImGui::EndTable();
	}
	ImGui::PopStyleVar(2);

	// Button to create new client configs
	{
		ImGui::NewLine();
		if( ImGui::Button("Add Client Config") && !gPopup_ClientConfig_pConfig ) 
		{ 
			gPopup_ClientConfig_pConfig = NetImgui::Internal::netImguiNew<NetImguiServer::Config::Client>(); 
		}
		if( ImGui::IsItemHovered() ){
			ImGui::SetTooltip(	"Clients configurations let this 'NetImgui Server' attempt connection to waiting clients (Server -> Client).\n"\
								"Clients without configuration can also connect directly to this server on port : %i (Server <- Client).", static_cast<int>(NetImguiServer::Config::Server::sPort));
		}

				
		ImGui::SameLine(0.f, 16.f); ImGui::TextUnformatted("Note:");
		ImGui::SameLine(); ImGui::TextColored(ImVec4(0.75f,0.75f,0.75f,1.f), "%i connected client(s)", connectedClient);
		if( connectedClient == 0 && ImGui::IsItemHovered() )
		{
			ImGui::SetTooltip(	"Make sure your remote client either:"\
								"\n -Attempts connection to this server on port (%i)"\
								"\n -Waits connection from the server (after being added to client configs list)", NetImguiServer::Config::Server::sPort);
		}		
	}
}

//=================================================================================================
// Display some relevenant stats in the MainMenu bar
//=================================================================================================
void DrawImguiContent_MainMenu_Stats()
{	
	constexpr float width(100.f);
	uint32_t txKBs(0), rxKBs(0), connected(0);	
	for(uint32_t i(0); i<RemoteClient::Client::GetCountMax(); ++i)
	{
		RemoteClient::Client& client = RemoteClient::Client::Get(i);
		txKBs		+= client.mbIsConnected ? client.mStatsSentBps/1024 : 0;
		rxKBs		+= client.mbIsConnected ? client.mStatsRcvdBps/1024 : 0;
		connected	+= client.mbIsConnected ? 1 : 0;
	}

	ImGui::SameLine(0.f, ImGui::GetContentRegionAvailWidth()-200.f);
	ImGui::TextColored(kColorContent, "(Rx) %iKB/s   (Tx) %iKB/s", rxKBs, txKBs);
	if (ImGui::IsItemHovered())
	{
		uint64_t txData(NetImguiServer::Network::GetStatsDataSent());
		uint64_t rxData(NetImguiServer::Network::GetStatsDataRcvd());		
		uint8_t txUnitIdx = ConvertDataAmount(txData);
		uint8_t rxUnitIdx = ConvertDataAmount(rxData);	
		ImGui::BeginTooltip();
		{
			ImGui::TextUnformatted("Connections");		ImGui::SameLine(width); ImGui::TextColored(kColorContent, ": %i", connected);
			ImGui::TextUnformatted("Data Received");	ImGui::SameLine(width); ImGui::TextColored(kColorContent, ": %i %s", static_cast<int>(rxData), kDataSizeUnits[rxUnitIdx]);
			ImGui::TextUnformatted("Data Sent");		ImGui::SameLine(width); ImGui::TextColored(kColorContent, ": %i %s", static_cast<int>(txData), kDataSizeUnits[txUnitIdx]);			
		}
		ImGui::EndTooltip();
	}
}

//=================================================================================================
// Generate the entries for the main menubar
//=================================================================================================
void DrawImguiContent_MainMenu()
{	
	if( ImGui::BeginMainMenuBar() )
	{		
		ImGui::SetNextWindowSize(ImVec2(ImGui::GetContentRegionAvailWidth(),0)); // Will let Menu popup content fill the screen width
		if (ImGui::BeginMenu("Clients"))
		{
			DrawImguiContent_MainMenu_Clients();
			ImGui::EndMenu();
		}
		
		gPopup_ServerConfig_Show	^= ImGui::MenuItem("Settings");
		gPopup_AboutNetImgui_Show	^= ImGui::MenuItem("About");
		DrawImguiContent_MainMenu_Stats();
		ImGui::EndMainMenuBar();
	}
}

//=================================================================================================
// Main entry point for rendering all of our NetImguiServer UI
//=================================================================================================
ImVec4 DrawImguiContent()
{   	
	const ImVec4 viewportTint(0.5f, 0.5f, 0.5f, 1.f);

	Popup_ServerConfig();
	Popup_ClientConfigEdit();
	Popup_ClientConfigDelete();
	Popup_ConfirmDisconnect();
	Popup_AboutNetImgui();

	DrawImguiContent_MainMenu();
	DrawImguiContent_SetupDocking(viewportTint);
	DrawImguiContent_Clients();	
	//ImGui::ShowDemoWindow();

	return ImVec4(gBackgroundColor.x*viewportTint.x, gBackgroundColor.y*viewportTint.y, gBackgroundColor.z*viewportTint.z, 1.f);
}


//=================================================================================================
// Startup : Initialize resources used by out server UI
//=================================================================================================
bool Startup()
{
	int Width(0), Height(0), Channel(0);
	stbi_uc* pBGPixels = stbi_load("Background.png", &Width, &Height, &Channel, 0);
	if( pBGPixels )
	{		
		// @Sammyfreg TODO : Support multiple format for Background
		gvBackgroundTextureSize.x = static_cast<float>(Width);
		gvBackgroundTextureSize.y = static_cast<float>(Height);
		NetImguiServer::App::HAL_CreateTexture(uint16_t(Width), uint16_t(Height), NetImgui::eTexFormat::kTexFmtRGBA8, pBGPixels, gpHAL_BackgroundTexture);
		gBackgroundColor.x	= static_cast<float>(pBGPixels[0]) / 255.f;
		gBackgroundColor.y	= static_cast<float>(pBGPixels[1]) / 255.f;
		gBackgroundColor.z	= static_cast<float>(pBGPixels[2]) / 255.f;
		gBackgroundColor.w	= 1.f;
		stbi_image_free(pBGPixels);
	}

	ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_Right;
	ImGui::GetStyle().TabBorderSize = 1.f;
	return true;
}

//=================================================================================================
// Shutdown : Free resources
//=================================================================================================
void Shutdown()
{
	NetImguiServer::App::HAL_DestroyTexture(gpHAL_BackgroundTexture);	
}

}} // namespace NetImguiServer { namespace UI
