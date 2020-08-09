#include "stdafx.h"
#include <stdio.h>
#include <algorithm>

#include <shellapi.h>	// To open webpage link
#include <NetImgui_Api.h>
#include "ServerInfoTab.h"
#include "ClientRemote.h"
#include "ClientConfig.h"

constexpr char	kNetImguiURL[]	= "https://github.com/sammyfreg/netImgui";
bool			gShowAbout		= false;

//=================================================================================================
// Edit an existing or new Client Config entry
void ServerInfoTab_DrawClient_SectionConfig_PopupEdit(ClientConfig*& pEditClientCfg)
//=================================================================================================
{	
	bool bOpenEdit(pEditClientCfg);
	if (bOpenEdit)
	{		
		ImGui::OpenPopup("Edit Client Info");
		if (ImGui::BeginPopupModal("Edit Client Info", &bOpenEdit, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::NewLine();
			// --- Name ---
			ImGui::InputText("Display Name", pEditClientCfg->ClientName, sizeof(pEditClientCfg->ClientName));
			
			// --- IP Address ---
			ImGui::InputText("Host Name", pEditClientCfg->HostName, sizeof(pEditClientCfg->HostName));
			if( ImGui::IsItemHovered() ){
				ImGui::SetTooltip("IP address or name of the host to reach");
			}

			// --- Port ---
			int port = static_cast<int>(pEditClientCfg->HostPort);
			if( ImGui::InputInt("Host Port", &port, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue) ){
				pEditClientCfg->HostPort = std::min<int>(0xFFFF, std::max<int>(1, port));
			}
			ImGui::SameLine();
			if (ImGui::Button("Default")){
				pEditClientCfg->HostPort = NetImgui::kDefaultClientPort;
			}

			// --- Auto ---
			ImGui::Checkbox("Auto Connect", &pEditClientCfg->ConnectAuto);
			
			// --- Save/Cancel ---
			ImGui::NewLine();
			ImGui::Separator();
			bOpenEdit &= !ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvailWidth()/2.f, 0));
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Save", ImVec2(ImGui::GetContentRegionAvailWidth(), 0))){
				ClientConfig::SetConfig(*pEditClientCfg);
				ClientConfig::SaveAll();
				bOpenEdit = false;
			}
			ImGui::EndPopup();
		}
	}	

	if( !bOpenEdit ){
		SafeDelete(pEditClientCfg);
	}
}

//=================================================================================================
// Client Config Delete Confirmation
void ServerInfoTab_DrawClient_SectionConfig_PopupDelete(uint32_t& ClientId)
//=================================================================================================
{
	if( ClientId != ClientConfig::kInvalidRuntimeID )
	{
		ClientConfig config;
		bool bOpenDelConfirm( ClientConfig::GetConfigByID(ClientId, config) );
		if (bOpenDelConfirm)
		{
			ImGui::OpenPopup("Confirmation##DEL");
			if (ImGui::BeginPopupModal("Confirmation##DEL", &bOpenDelConfirm, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::NewLine();
				ImGui::TextUnformatted("Are you certain you want to remove\nthis client configuration ?");
				ImGui::SetNextItemWidth(-1.0f);
				if (ImGui::ListBoxHeader("##", 1, 2))
				{				
					ImGui::TextUnformatted(config.ClientName);
					ImGui::ListBoxFooter();
				}

				ImGui::NewLine();
				ImGui::Separator();
				bOpenDelConfirm &= !ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvailWidth()/2.f, 0));
				ImGui::SetItemDefaultFocus();

				ImGui::SameLine();
				if (ImGui::Button("Yes", ImVec2(ImGui::GetContentRegionAvailWidth(), 0))){
					ClientConfig::DelConfig(ClientId);
					ClientConfig::SaveAll();
					bOpenDelConfirm = false;
				}
				ImGui::EndPopup();
			}
		}

		if( !bOpenDelConfirm ){
			ClientId = ClientConfig::kInvalidRuntimeID;
		}
	}
}

//=================================================================================================
void ServerInfoTab_DrawClients_SectionConnected()
//=================================================================================================
{
	float widthAjust = std::max<float>(50.f, ImGui::GetContentRegionAvailWidth() - 250.f) / 2.f;
	ImGui::NewLine();
	ImGui::Separator();
	ImGui::Text("Connected");
	ImGui::Separator();
	ImGui::Columns(5, "ColumnConnected", false);
	ImGui::SetColumnWidth(0, widthAjust);	ImGui::SetColumnWidth(1, widthAjust);	ImGui::SetColumnWidth(2, 50);	ImGui::SetColumnWidth(3, 70); ImGui::SetColumnWidth(4, 120);

	ImGui::Selectable("Name", true, ImGuiSelectableFlags_SpanAllColumns); ImGui::NextColumn();
	ImGui::TextUnformatted("IP"); ImGui::NextColumn();
	ImGui::TextUnformatted("Port"); ImGui::NextColumn();
	ImGui::TextUnformatted("Duration"); ImGui::NextColumn();
	ImGui::NextColumn();

	// Skipping 1st element, because it's the ServerTab
	for (uint32_t i(1); i < ClientRemote::GetCountMax(); ++i)
	{
		ClientRemote& client = ClientRemote::Get(i);
		if( client.mbIsConnected )
		{
			ImGui::PushID(i);
			ImGui::Selectable(client.mName, false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(widthAjust * 2 + 105, 0)); ImGui::NextColumn();
			ImGui::TextUnformatted(client.mConnectHost); ImGui::NextColumn();
			ImGui::Text("%5i", client.mConnectPort); ImGui::NextColumn();
			ImGui::Text("10:00"); ImGui::NextColumn();
			if( !client.mbPendingDisconnect ){
				client.mbPendingDisconnect = ImGui::Button("Disconnect");
			}ImGui::NextColumn();
			ImGui::PopID();
		}
	}
	ImGui::Columns(1);
}

//=================================================================================================
void ServerInfoTab_DrawClients_SectionConfig()
//=================================================================================================
{
	// Popups to edit or delete Client Configurations
	static uint32_t PendingDelete			= ClientConfig::kInvalidRuntimeID;
	static ClientConfig* pPendingEditCfg	= nullptr;
	ServerInfoTab_DrawClient_SectionConfig_PopupEdit(pPendingEditCfg);
	ServerInfoTab_DrawClient_SectionConfig_PopupDelete(PendingDelete);


	// Display list of Client Configuration
	ClientConfig clientConfig;
	float widthAjust = std::max<float>(50.f, ImGui::GetContentRegionAvailWidth() - 250.f) / 2.f;
	ImGui::NewLine();
	ImGui::Separator();
	ImGui::Text("Client Configurations");
	ImGui::Separator();
	ImGui::Columns(5, "ColumnConfig", false);
	ImGui::SetColumnWidth(0, widthAjust);	ImGui::SetColumnWidth(1, widthAjust);	ImGui::SetColumnWidth(2, 50);	ImGui::SetColumnWidth(3, 50);	ImGui::SetColumnWidth(4, 150);
	ImGui::Selectable("Name", true, ImGuiSelectableFlags_SpanAllColumns, ImVec2(widthAjust * 2 + 80, 0)); ImGui::NextColumn();
	ImGui::TextUnformatted("Hostname"); ImGui::NextColumn();
	ImGui::TextUnformatted("Port"); ImGui::NextColumn();
	ImGui::TextUnformatted("Auto"); ImGui::NextColumn();
	if (ImGui::Button("+") && !pPendingEditCfg) pPendingEditCfg = new ClientConfig; ImGui::NextColumn();
	
	for (int i = 0; ClientConfig::GetConfigByIndex(i, clientConfig); ++i)
	{
		ImGui::PushID(i);
		ImGui::PushStyleColor(ImGuiCol_CheckMark, clientConfig.Connected ? ImVec4(0.7f, 1.f, 0.25f, 1.f) : ImVec4(1.f, 0.35f, 0.35f, 1.f));
		ImGui::RadioButton("##Connected", true);
		ImGui::SameLine(); ImGui::PopStyleColor();
		ImGui::Selectable(clientConfig.ClientName, false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(widthAjust * 2 + 40, 0)); ImGui::NextColumn();

		ImGui::TextUnformatted(clientConfig.HostName); ImGui::NextColumn();
		ImGui::Text("%5i", clientConfig.HostPort); ImGui::NextColumn();

		// Edit: AutoConnect changed
		if (ImGui::Checkbox("##auto", &clientConfig.ConnectAuto))
		{
			ClientConfig::SetProperty_ConnectAuto(clientConfig.RuntimeID, clientConfig.ConnectAuto);
			ClientConfig::SaveAll();
		}ImGui::NextColumn();

		// Edit: Remove client
		if (ImGui::Button("-"))
			PendingDelete = clientConfig.RuntimeID;

		ImGui::SameLine(); if (ImGui::Button("..."))
		{
			pPendingEditCfg = new ClientConfig;
			*pPendingEditCfg = clientConfig;
		}
		// Edit: Connection attempt
		if (!clientConfig.Connected && !clientConfig.ConnectRequest)
		{
			ImGui::SameLine(); if (ImGui::Button("Connect"))
			{
				ClientConfig::SetProperty_ConnectRequest(clientConfig.RuntimeID, true);
			}
		}ImGui::NextColumn();

		ImGui::PopID();
	}
	ImGui::Columns(1);
}

//=================================================================================================
// Window displaying active connexion and Client list configured to attempt connexion with
void ServerInfoTab_DrawClients()
//=================================================================================================
{
	if( ImGui::Begin("Clients") )
	{   
		ServerInfoTab_DrawClients_SectionConnected();		
		ServerInfoTab_DrawClients_SectionConfig();
	}
	ImGui::End();
}

//=================================================================================================
void ServerInfoTab_AboutDialog()
//=================================================================================================
{
	static bool sbURLHover = false;
	if( gShowAbout )
	{
		if( ImGui::Begin("About netImgui", &gShowAbout, ImGuiWindowFlags_AlwaysAutoResize) )
		{
			ImGui::NewLine();
			ImGui::Text("Version : %s", NETIMGUI_VERSION);
			ImGui::NewLine();
			ImGui::Text("For more informations : ");
			
			ImGui::TextColored(sbURLHover ? ImColor(0.8f, 0.8f, 1.f,1.f) : ImColor(0.5f, 0.5f, 1.f,1.f), kNetImguiURL);
			sbURLHover = ImGui::IsItemHovered();
			if( sbURLHover && ImGui::IsMouseClicked(ImGuiMouseButton_Left) )
				ShellExecuteA(0, 0, kNetImguiURL, 0, 0 , SW_SHOW );

			ImGui::NewLine();
			ImGui::Separator();
			if( ImGui::Button("Close", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)) ) 
				gShowAbout = false;
		}
		ImGui::End();
	}
	else
		sbURLHover = false;
}
//=================================================================================================
// Draw
//=================================================================================================
void ServerInfoTab_Draw(uint32_t clientCount)
{
	if (NetImgui::NewFrame())
	{
		ImGui::BeginMainMenuBar();
		if( ImGui::Button("About") ) gShowAbout = true;
		ImGui::Text(clientCount <= 1 ? "Waiting for Connection..." : "Client connected : %i", clientCount - 1);
		ImGui::EndMainMenuBar();
		ServerInfoTab_DrawClients();
		ServerInfoTab_AboutDialog();
		NetImgui::EndFrame();
	}
}

//=================================================================================================
// Startup
//=================================================================================================
bool ServerInfoTab_Startup(unsigned int ServerPort)
{	
	if( !NetImgui::Startup() )
		return false;

	ImGui::SetCurrentContext(ImGui::CreateContext());
	ImGuiIO& io = ImGui::GetIO();	
	
	// Build
	unsigned char* pixels;
	int width, height;
	io.Fonts->AddFontDefault();
	io.Fonts->Build();
	io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
	NetImgui::SendDataTexture(0, pixels, static_cast<uint16_t>(width), static_cast<uint16_t>(height), NetImgui::eTexFormat::kTexFmtA8);	   

	// Store our identifier
	io.Fonts->TexID = reinterpret_cast<ImTextureID*>(0);

	// Cleanup (don't clear the input data if you want to append new fonts later)
	io.Fonts->ClearInputData();
	io.Fonts->ClearTexData();

	if( NetImgui::ConnectToApp("netImgui", "localhost", ServerPort, false) )
	{
		// Wait for 'ServerInfoTab' client to connect first in slot 0
		while( !ClientRemote::Get(0).mbIsConnected )
			std::this_thread::yield();
		return true;
	}
	return false;
}

//=================================================================================================
// Shutdown
//=================================================================================================
void ServerInfoTab_Shutdown()
{	
	ImGui::DestroyContext(ImGui::GetCurrentContext());
}
