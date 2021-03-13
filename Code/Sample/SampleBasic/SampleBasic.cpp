//=================================================================================================
// SAMPLE BASIC
//-------------------------------------------------------------------------------------------------
// Barebone example of adding NetImgui to a code base. This demonstrate how little changes
// are needed to be up and running.
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
	// (1) Start a new Frame.
	// Note:	With ImGui 1.81+ NetImgui can automatically intercept Imgui::NewFrame/Render. This
	//			sample does this. For older Imgui releases, please look at 'Client_Draw_ModeAlways'
	//			in 'SampleNewFrame' on how to tell NetImgui directly about NewFrame/EndFrame.
	//			Other samples also avoid the auto intercept to allow drawing only when needed.
	//---------------------------------------------------------------------------------------------
	ImGui::NewFrame();

	//-----------------------------------------------------------------------------------------
	// (2) Draw ImGui Content
	//-----------------------------------------------------------------------------------------
	ClientUtil_ImGuiContent_Common("SampleBasic"); //Note: Connection to remote server done in there

	ImGui::SetNextWindowPos(ImVec2(32,48), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(400,400), ImGuiCond_Once);
	if( ImGui::Begin("Sample Basic", nullptr) )
	{
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Basic demonstration of netImgui code integration.");		
		ImGui::TextWrapped("Create a basic Window with some text.");
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
	// (3) Finish the frame
	// Note:	Same note as in (1)
	//---------------------------------------------------------------------------------------------
	ImGui::Render();

	//---------------------------------------------------------------------------------------------
	// (4) Return content to draw by local renderer. Stop drawing locally when remote connected
	//---------------------------------------------------------------------------------------------
	return !NetImgui::IsConnected() ? ImGui::GetDrawData() : nullptr;	
}

} // namespace SampleClient
