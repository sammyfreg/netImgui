//=================================================================================================
// SAMPLE Single Include
//-------------------------------------------------------------------------------------------------
// Identical to 'SampleBasic' except for how the NetImgui code is integrated to the project.
//
// Other samples use the 'NetImgui_Api.h' and link against a pre compiled 'NetImgui' library.
//
// This sample uses the define 'NETIMGUI_IMPLEMENTATION' before including 'NetImgui_Api.h'. 
// This tell 'NetImgui_Api.h' to also include every 'NetImgui' sources file,  
// removing the need to link against the library.
//
// Note: Another (more conventional) way of compiling 'NetImgui' with your code, 
//		 is to includes its sources file directly in your project. This single header include
//		 approach was added for potential convenience, minimizing changes to a project.
//=================================================================================================

// Defining this value before '#include <NetImgui_Api.h>', also load all 'NetImgui' client sources
// It should only be done in 1 source file (to avoid duplicate symbol at link time), 
// other location can still include 'NetImgui_Api.h', but without using the define
#define NETIMGUI_IMPLEMENTATION
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
	NetImgui::Shutdown();
}

//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
ImDrawData* Client_Draw()
{
	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame.
	//---------------------------------------------------------------------------------------------
	ImGui::NewFrame();
	
	//-----------------------------------------------------------------------------------------
	// (2) Draw ImGui Content 		
	//-----------------------------------------------------------------------------------------
	ClientUtil_ImGuiContent_Common("SampleSingleInclude"); //Note: Connection to remote server done in there

	ImGui::SetNextWindowPos(ImVec2(32,48), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(400,400), ImGuiCond_Once);
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

	//---------------------------------------------------------------------------------------------
	// (4) Return content to draw by local renderer. Stop drawing locally when remote connected
	//---------------------------------------------------------------------------------------------
	return !NetImgui::IsConnected() ? ImGui::GetDrawData() : nullptr;	
}

} // namespace SampleClient
