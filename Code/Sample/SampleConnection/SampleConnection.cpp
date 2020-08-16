//=================================================================================================
// SAMPLE BASIC
//-------------------------------------------------------------------------------------------------
// Example of the 2 methods for connecting a netImgui client and server together.
// Can either try to reach the server directly, or wait for the server to connect to this client.
//=================================================================================================

#include <NetImgui_Api.h>
#include <thread>
#include "..\Common\Sample.h"
#include "..\Common\WarningDisable.h"

namespace SampleClient
{

//=================================================================================================
// HELPER FUNCTION
//
// Help start a new ImGui drawing frame, by calling the appropriate function, based on whether 
// we are connected to netImgui server or not.
//
// This function is intended when user intend to display ImGui either locally or remotely, 
// but not both.
//
// Returns True when we need to draw a new ImGui frame 
// (netImgui doesn't expect new content every frame)
//=================================================================================================
bool NetImguiHelper_NewFrame()
{
	if (NetImgui::IsConnected())
	{
		return NetImgui::NewFrame();
	}
	ImGui::NewFrame();
	return true;
}

//=================================================================================================
// HELPER FUNCTION
//
// Help end a ImGui drawing frame, by calling the appropriate function, based on whether we are
// connected to netImgui server or not.
//
// This function is intended when user intend to display ImGui either locally or remotely, 
// but not both.
//
// Returns True when we need render this new ImGui frame locally
//=================================================================================================
bool NetImguiHelper_EndFrame()
{
	if (NetImgui::IsConnected())
	{
		if (NetImgui::IsRemoteDraw())
			NetImgui::EndFrame();

		return false;
	}

	ImGui::Render();
	return true;
}

//=================================================================================================
//
//=================================================================================================
bool Client_Startup()
{
	if (!NetImgui::Startup())
		return false;

	// Can have more ImGui initialization here, like loading extra fonts.
	// ...

	return true;
}

//=================================================================================================
//
//=================================================================================================
void Client_Shutdown()
{
	NetImgui::Shutdown();
}

//=================================================================================================
// Added a call to this function in 'ImGui_ImplDX11_CreateFontsTexture()', allowing us to 
// forward the Font Texture information to netImgui.
//=================================================================================================
void Client_AddFontTexture(uint64_t texId, void* pData, uint16_t width, uint16_t height)
{
	NetImgui::SendDataTexture(texId, pData, width, height, NetImgui::eTexFormat::kTexFmtRGBA8);
}

//=================================================================================================
//! @brief		Custom Connect thread example
//! @details	This function demonstrate providing your own function to start a new thread 
//!				that will handle communication with remote netImgui application. 
//!				The default implementation also use std::thread
//=================================================================================================
void CustomCommunicationThread(void ComFunctPtr(void*), void* pClient)
{
	std::thread(ComFunctPtr, pClient).detach();
}

//=================================================================================================
// Window with connection settings
//=================================================================================================
void Client_Draw_WindowConnection()
{
	static char sServerHostName[64] = { 'l','o','c','a','l','h','o','s','t',0 };
	static int sServerHostPort		= NetImgui::kDefaultServerPort;
	static int sClientHostPort		= NetImgui::kDefaultClientPort;

	ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);
	if (ImGui::Begin("Sample Connection", nullptr))
	{
		//-----------------------------------------------------------------------------------------
		// We are connected to netImgui Server
		//-----------------------------------------------------------------------------------------
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Demonstration of netImgui connection code.");
		if (NetImgui::IsConnected())
		{
			ImGui::TextWrapped("We are connected!");
			if (ImGui::Button("Disconnect"))
			{
				NetImgui::Disconnect();
			}
		}

		//-----------------------------------------------------------------------------------------
		// We are waiting for the netImgui server to connect to us
		//-----------------------------------------------------------------------------------------
		else if (NetImgui::IsConnectionPending())
		{
			ImGui::TextWrapped("Waiting on port '%i' for Server request to connect with us. You can add a new 'Client Configuration' on the netImgui server application, or launch the application with the commandline 'netImguiServer (ClientHostName);(ClientPort)'.", sClientHostPort);
			ImGui::NewLine();
			ImGui::TextUnformatted("Example : netImguiServer 127.0.0.1;8889");
			ImGui::NewLine();
			if (ImGui::Button("Cancel"))
			{
				NetImgui::Disconnect();
			}
		}
		else
		{
			//-------------------------------------------------------------------------------------
			// Config to try connecting directly to the netImgui Server
			//-------------------------------------------------------------------------------------
			ImGui::TextWrapped("This application attempts establishing a connection to a netImgui server. Once connection is established, the UI will only be displayed remotely, with no content in this client while connected.");
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1, 1, 1, 0.1));
			ImGui::NewLine();
			{
				ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Connect to the server");
				ImGui::BeginChild("ConnectTo", ImVec2(350, 50), 0);
				{
					ImGui::InputText("HostName", sServerHostName, sizeof(sServerHostName));
					ImGui::InputInt("HostPort", &sServerHostPort);
					ImGui::EndChild();
				}
				if (ImGui::Button("Connect To", ImVec2(120, 0)))
				{
					NetImgui::ConnectToApp(CustomCommunicationThread, "SampleConnect", sServerHostName, sServerHostPort);	// Launch Connection to server
				}
			}

			ImGui::NewLine();
			ImGui::Text("Or");
			ImGui::NewLine();

			//-------------------------------------------------------------------------------------
			// Config to wait for a connection fro the netImgui Server
			//-------------------------------------------------------------------------------------
			{
				ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Wait connection from the server");
				ImGui::BeginChild("ConnectFrom", ImVec2(350, 50), 0);
				{
					ImGui::InputInt("HostPort", &sClientHostPort);
					ImGui::EndChild();
				}
				if (ImGui::Button("Connect Wait", ImVec2(120, 0)))
				{
					NetImgui::ConnectFromApp(CustomCommunicationThread, "SampleConnect", sClientHostPort);	// Launch connection waiting from server
				}
			}
			ImGui::PopStyleColor();
		}
	}
	ImGui::End();
}

//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
ImDrawData* Client_Draw()
{
	//---------------------------------------------------------------------------------------------
	// (1) Prepare a new Frame
	// Note: When remote drawing, it is possible for 'NetImgui::NewFrame()' to return false, 
	//		 in which case we should just skip refreshing the ImGui content.
	//---------------------------------------------------------------------------------------------
	if (NetImguiHelper_NewFrame())
	{
		//-----------------------------------------------------------------------------------------
		// (2) Draw ImGui Content 		
		//-----------------------------------------------------------------------------------------
		Client_Draw_WindowConnection();
		ImGui::ShowDemoWindow();
			
		//-----------------------------------------------------------------------------------------
		// (3a) Prepare the data for rendering locally (when not connected), or...
		// (3b) Send the data to the netImGui server (when connected)
		//-----------------------------------------------------------------------------------------
		if (NetImguiHelper_EndFrame())
		{
			//-----------------------------------------------------------------------------------------
			// (4a) Let the local DX11 renderer know about the UI to draw (when not connected)
			//		Could also rely on '!NetImgui::IsRemoteDraw()' instead of return value of 
			//		'NetImguiHelper_EndFrame()'
			//-----------------------------------------------------------------------------------------
			return ImGui::GetDrawData();
		}
	}

	//---------------------------------------------------------------------------------------------
	// (4b) Render nothing locally (when connected)
	//---------------------------------------------------------------------------------------------
	return nullptr;
}

} // namespace SampleClient
