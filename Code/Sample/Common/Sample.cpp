//=================================================================================================
// SAMPLE 
//-------------------------------------------------------------------------------------------------
// Common code shared by all samples 
//=================================================================================================

#include <NetImgui_Api.h>
#include "..\Common\Sample.h"

namespace SampleClient
{

static int sClientPort				= NetImgui::kDefaultClientPort;
static int sServerPort				= NetImgui::kDefaultServerPort;
static char sServerHostname[128]	= {"localhost"};
static bool sbShowDemoWindow		= false;

void ClientUtil_ImGuiContent_Common(const char* zAppName, bool bCreateNewContext)
{
	(void)bCreateNewContext;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3,6) );
	if( ImGui::BeginMainMenuBar() )
	{				
		ImGui::AlignTextToFramePadding();
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "%s", zAppName);
		ImGui::SameLine(0,32);
		
		//-----------------------------------------------------------------------------------------
		if( NetImgui::IsConnected() )
		//-----------------------------------------------------------------------------------------
		{
			ImGui::TextUnformatted("Status: Connected");
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));
			ImGui::SetCursorPosY(3);
			if( ImGui::Button("Disconnect", ImVec2(120,0)) ) 
			{
				NetImgui::Disconnect();
			}
			ImGui::PopStyleVar();
		}
		
		//-----------------------------------------------------------------------------------------
		else if( NetImgui::IsConnectionPending() )
		//-----------------------------------------------------------------------------------------
		{
			ImGui::TextUnformatted("Status: Waiting Server");
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));
			ImGui::SetCursorPosY(3);
			if (ImGui::Button("Cancel", ImVec2(120,0)))
			{
				NetImgui::Disconnect();
			}
			ImGui::PopStyleVar();
		}

		//-----------------------------------------------------------------------------------------
		else // No connection
		//-----------------------------------------------------------------------------------------
		{
			//-------------------------------------------------------------------------------------
			if( ImGui::BeginMenu("[ Connect to ]") )
			//-------------------------------------------------------------------------------------
			{
				ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Server Settings");
				ImGui::InputText("Hostname", sServerHostname, sizeof(sServerHostname));
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Address of PC running the netImgui server application. Can be an IP like 127.0.0.1");
				ImGui::InputInt("Port", &sServerPort);
				ImGui::NewLine();
				ImGui::Separator();
				if (ImGui::Button("Connect", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				{
					NetImgui::ConnectToApp(zAppName, sServerHostname, sServerPort, bCreateNewContext);
				}
				ImGui::EndMenu();
			}

			if( ImGui::IsItemHovered() )
				ImGui::SetTooltip("Attempt a connection to a remote netImgui server at the provided address.");

			//-------------------------------------------------------------------------------------
			if (ImGui::BeginMenu("[  Wait For  ]"))
			//-------------------------------------------------------------------------------------
			{
				ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Client Settings");				
				ImGui::InputInt("Port", &sClientPort);
				ImGui::NewLine();
				ImGui::Separator();
				if (ImGui::Button("Listen", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				{
					NetImgui::ConnectFromApp(zAppName, sClientPort, bCreateNewContext);
				}
				ImGui::EndMenu();
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Start listening for a connection request by a remote netImgui server, on the provided Port.");
		}
		
		ImGui::SameLine(0,40);	
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8,0.8,0.8,0.9) );
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));				
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, sbShowDemoWindow ? 1.f : 0.f);
		ImGui::SetCursorPosY(3);
		if( ImGui::Button("Show ImGui Demo", ImVec2(120,0)) )
		{
			sbShowDemoWindow = !sbShowDemoWindow;
		}		
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleVar();

	if( sbShowDemoWindow )
	{
		ImGui::ShowDemoWindow(&sbShowDemoWindow);
	}
}

} // namespace SampleClient
