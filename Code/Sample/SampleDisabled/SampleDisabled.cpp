//=================================================================================================
// SAMPLE DISABLED
//-------------------------------------------------------------------------------------------------
// This sample demonstrate compiling your code with netImgui disabled but ImGui still active.
// It relies on the Define 'NETIMGUI_ENABLED' assigned in the project properties to compile 
// with NetImfui inactive.
//=================================================================================================

#include <NetImgui_Api.h>
#include "..\Common\Sample.h"

namespace SampleClient
{
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
	NetImgui::Shutdown(true);
}

//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
ImDrawData* Client_Draw()
{
	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame
	//---------------------------------------------------------------------------------------------
	NetImgui::NewFrame();
	
	//-----------------------------------------------------------------------------------------
	// (2) Draw ImGui Content 		
	//-----------------------------------------------------------------------------------------	
	ClientUtil_ImGuiContent_Common("SampleDisabled");
	ImGui::SetNextWindowPos(ImVec2(32,48), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(400,400), ImGuiCond_Once);
	if( ImGui::Begin("Sample Basic", nullptr) )
	{
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Compiling with netImgui Disabled.");		
		ImGui::TextWrapped(	"This shows being able to continue using ImGui normally, while netImgui code has been disabled. "
							"No connection with the remote netImgui will be possible since the client code is entirely ifdef out "
							"and the netImgui client API left with shell content calling Dear Imgui directly.");
		ImGui::NewLine();
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Where are we drawing: ");
		ImGui::SameLine();
		ImGui::TextUnformatted(NetImgui::IsDrawingRemote() ? "Remote Draw" : "Local Draw");
		ImGui::NewLine();
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Filler content");
		ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
	}
	ImGui::End();
	
	//---------------------------------------------------------------------------------------------
	// (3) Finish the frame, preparing the drawing data and...
	// (3a) Send the data to the netImGui server when connected
	//---------------------------------------------------------------------------------------------
	NetImgui::EndFrame();
	
	//---------------------------------------------------------------------------------------------
	// (4) Forward to drawing data our local renderer when not remotely drawing
	//---------------------------------------------------------------------------------------------
	return !NetImgui::IsConnected() ? ImGui::GetDrawData() : nullptr;
}

} // namespace SampleClient
