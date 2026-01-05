//=================================================================================================
// SAMPLE Single Include
//-------------------------------------------------------------------------------------------------
// Identical to 'SampleBasic' except for how the NetImgui code is integrated to the project.
//
// Other samples use the 'NetImgui_Api.h' and link against a pre compiled 'NetImgui' library.
//
// By defining 'NETIMGUI_IMPLEMENTATION' before '#include <NetImgui_Api.h>', you can tell the 
// library to pull all source files needed to compile NetImgui here.
//
// It should only be done in 1 source file (to avoid duplicate symbol at link time).
//
// Other location can still include 'NetImgui_Api.h', but without using the define
//
// Note: Another (more conventional) way of compiling 'NetImgui' with your code, 
//		 is to includes its sources file directly in your project. This single header include
//		 approach was added for potential convenience, minimizing changes to a project, but
//		 can prevent code change detection in these included files, when compiling.
//=================================================================================================
#define NETIMGUI_IMPLEMENTATION
#include <NetImgui_Api.h>
#include "../Common/Sample.h"

//=================================================================================================
// SAMPLE CLASS
//=================================================================================================
class SampleSingleInclude : public Sample::Base
{
public:
				SampleSingleInclude() : Base("SampleSingleInclude") {}
virtual void	Draw() override;
};

//=================================================================================================
// GET SAMPLE
// Each project must return a valid sample object
//=================================================================================================
Sample::Base& GetSample()
{
	static SampleSingleInclude sample;
	return sample;
}

//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
void SampleSingleInclude::Draw()
{
	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame.
	//---------------------------------------------------------------------------------------------
	ImGui::NewFrame();
	
	//-----------------------------------------------------------------------------------------
	// (2) Draw ImGui Content 		
	//-----------------------------------------------------------------------------------------
	Base::Draw_Connect(); //Note: Connection to remote server done in there

	ImGui::SetNextWindowPos(ImVec2(32,48), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(600,500), ImGuiCond_Once);
	if( ImGui::Begin("Sample Single Include", nullptr) )
	{
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Basic demonstration of NetImgui code integration.");
		ImGui::TextWrapped("Create a basic Window with some text.");
		ImGui::NewLine();
		ImGui::TextWrapped("Identical to SampleBasic, the only difference is how the client code was included in the project.");
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
	//---------------------------------------------------------------------------------------------
	ImGui::Render();
}
