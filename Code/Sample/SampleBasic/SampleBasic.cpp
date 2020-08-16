//=================================================================================================
// SAMPLE BASIC
//-------------------------------------------------------------------------------------------------
// Barebone example of adding NetImgui to a code base. This demonstrate how little changes
// are needed to be up and running.
//=================================================================================================

#include <NetImgui_Api.h>
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
	if( NetImgui::IsConnected() )
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
	if( NetImgui::IsConnected() )
	{
		if( NetImgui::IsRemoteDraw() )
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
	if( !NetImgui::Startup() )
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
	NetImgui::SendDataTexture(texId, pData, width, height, NetImgui::eTexFormat::kTexFmtRGBA8 );
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
	if( NetImguiHelper_NewFrame() )	
	{
		//-----------------------------------------------------------------------------------------
		// (2) Draw ImGui Content 		
		//-----------------------------------------------------------------------------------------
		ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(600,250), ImGuiCond_Once); 
		if( ImGui::Begin("Sample Basic", nullptr) )
		{
			ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1),"Basic demonstration of netImgui code integration.");
			if( NetImgui::IsConnected() )
			{
				ImGui::TextWrapped("We are connected!");	
				if( ImGui::Button("Disconnect") )
				{
					NetImgui::Disconnect();
				}
			}
			else
			{
				ImGui::TextWrapped("This application attempts initiating connection to a netImgui server running on the same PC and port '%i'. Once connection is established, the UI will only be displayed remotely, with no content in this client while connected.", NetImgui::kDefaultServerPort);
				if (ImGui::Button("Connect", ImVec2(120, 0)))
				{
					NetImgui::ConnectToApp("SampleBasic", "localhost", NetImgui::kDefaultServerPort);
				}
			}

			ImGui::NewLine();
			ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Filler content unaffected by netImgui.");
			ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
		}
		ImGui::End();
		
		//-----------------------------------------------------------------------------------------
		// (3a) Prepare the data for rendering locally (when not connected), or...
		// (3b) Send the data to the netImGui server (when connected)
		//-----------------------------------------------------------------------------------------
		if( NetImguiHelper_EndFrame() )
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
