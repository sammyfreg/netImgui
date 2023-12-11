//=================================================================================================
// SAMPLE 
//-------------------------------------------------------------------------------------------------
// Common code shared by all samples 
//=================================================================================================

#include <NetImgui_Api.h>
#include "..\Common\Sample.h"

// Since NetImgui is disabled in this sample, it doesn't already include this header
#if !NETIMGUI_ENABLED
#include "imgui.h" 
namespace SampleClient { void ClientUtil_ImGuiContent_Common(const char*, NetImgui::ThreadFunctPtr, NetImgui::FontCreationFuncPtr){} }

#else

namespace SampleClient
{

static int sClientPort				= NetImgui::kDefaultClientPort;
static int sServerPort				= NetImgui::kDefaultServerPort;
static char sServerHostname[128]	= {"localhost"};
static bool sbShowDemoWindow		= false;

//=================================================================================================
// ClientUtil_ImGuiContent_Common
//-------------------------------------------------------------------------------------------------
// Function called by all samples, to display the Connection Options, and some other default
// MainMenu entries.
// 
// @param zAppName:				Name displayed in the Main Menu bar
// @param customThreadLauncher:	Optional thread launcher function to pass along NetImgui
// #param FontCreateFunction:	Optional font generation function to pass along NetImgui. Used to adjust to remote server monitor DPI
//=================================================================================================
void ClientUtil_ImGuiContent_Common(const char* zAppName, NetImgui::ThreadFunctPtr customThreadLauncher, NetImgui::FontCreationFuncPtr FontCreateFunction)
{
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
				if (ImGui::Button("Connect", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
				{
					NetImgui::ConnectToApp(zAppName, sServerHostname, sServerPort, customThreadLauncher, FontCreateFunction);
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
				if (ImGui::Button("Listen", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
				{
					NetImgui::ConnectFromApp(zAppName, sClientPort, customThreadLauncher, FontCreateFunction);
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
#endif
