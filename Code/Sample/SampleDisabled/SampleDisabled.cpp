//=================================================================================================
// SAMPLE DISABLED
//-------------------------------------------------------------------------------------------------
// This sample demonstrate compiling your code with netImgui disabled but ImGui still active.
// It relies on the Define 'NETIMGUI_ENABLED' assigned in the project properties to compile 
// with NetImfui inactive.
//=================================================================================================

#include <NetImgui_Api.h>
#include "imgui.h" // Since NetImgui is disabled in this sample, it will not include this header
#include "../Common/Sample.h"

//=================================================================================================
// SAMPLE CLASS
//=================================================================================================
class SampleDisabled : public Sample::Base
{
public:
					SampleDisabled() : Base("SampleDisabled") {}
virtual ImDrawData* Draw() override;
};

//=================================================================================================
// GET SAMPLE
// Each project must return a valid sample object
//=================================================================================================
Sample::Base& GetSample()
{
	static SampleDisabled sample;
	return sample;
}

//=================================================================================================
// DRAW
//=================================================================================================
ImDrawData* SampleDisabled::Draw()
{
	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame
	//---------------------------------------------------------------------------------------------
	ImGui::NewFrame();
	
	//-----------------------------------------------------------------------------------------
	// (2) Draw ImGui Content
	//-----------------------------------------------------------------------------------------		
	Base::Draw_Connect(); //Note: Connection to remote server done in there

	ImGui::SetNextWindowPos(ImVec2(32,48), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(400,400), ImGuiCond_Once);
	if( ImGui::Begin("Sample Disabled", nullptr) )
	{
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Compiling with netImgui Disabled.");		
		ImGui::TextWrapped(	"This shows being able to continue using ImGui normally, while netImgui code has been disabled. "
							"No connection with the remote netImgui will be possible since the client code is entirely ifdef out ");
		ImGui::NewLine();
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Filler content");
		ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
	}
	ImGui::End();
	
	//---------------------------------------------------------------------------------------------
	// (3) Finish the frame, preparing the drawing data and...
	//---------------------------------------------------------------------------------------------
	ImGui::Render();
	
	//---------------------------------------------------------------------------------------------
	// (4) Forward to drawing data our local renderer when not remotely drawing
	//---------------------------------------------------------------------------------------------
	return ImGui::GetDrawData();
}

