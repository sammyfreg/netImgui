//=================================================================================================
// SAMPLE DUAL UI
//-------------------------------------------------------------------------------------------------
// Example of supporting 2 differents ImGui display at the same time. One on the remote server
// and the second in the local client.
//=================================================================================================

#include <NetImgui_Api.h>
#include "..\Common\Sample.h"

namespace SampleClient
{
enum class eDisplayMode : int { LocalNone, LocalMirror, LocalIndependent };
static eDisplayMode		sDisplayMode			= eDisplayMode::LocalNone;
static ImGuiContext*	gpContextMain			= nullptr;
static ImGuiContext*	gpContextSide			= nullptr;

//=================================================================================================
//
//=================================================================================================
bool Client_Startup()
{
	if (!NetImgui::Startup())
		return false;

	gpContextMain	= ImGui::GetCurrentContext();					// Dear ImGui backend already creates a default context
	gpContextSide	= ImGui::CreateContext(ImGui::GetIO().Fonts);	// Creating a side context, used as a second display when connected
	return true;
}

//=================================================================================================
//
//=================================================================================================
void Client_Shutdown()
{			
	NetImgui::Shutdown(true);
	ImGui::DestroyContext(gpContextSide);
	ImGui::SetCurrentContext(gpContextMain);
	gpContextMain	= nullptr;
	gpContextSide	= nullptr;
}

//=================================================================================================
//
//=================================================================================================
void Client_Draw_LocalContent()
{	
	if( !NetImgui::IsDrawingRemote())
	{
		ImGui::NewLine();
		ImGui::TextWrapped("This text is only displayed locally.");
		ImGui::NewLine();
	}		
}

//=================================================================================================
//
//=================================================================================================
void Client_Draw_RemoteContent()
{
	if( NetImgui::IsDrawingRemote() )
	{
		ImGui::NewLine();
		ImGui::TextWrapped("This text is only displayed remotely (unless mirroring the output locally).");
	}
}

//=================================================================================================
//
//=================================================================================================
void Client_Draw_MainContent()
{
	ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Example of local/remote Imgui content");
	ImGui::TextWrapped("Demonstrate the ability of simultaneously displaying distinct content locally and remotely.");
	ImGui::NewLine();
	if (NetImgui::IsConnected())
	{
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Dual Display Mode:");
		ImGui::RadioButton("None", reinterpret_cast<int*>(&sDisplayMode), static_cast<int>(eDisplayMode::LocalNone)); ImGui::SameLine();
		ImGui::RadioButton("Mirror", reinterpret_cast<int*>(&sDisplayMode), static_cast<int>(eDisplayMode::LocalMirror)); ImGui::SameLine();
		ImGui::RadioButton("Independent", reinterpret_cast<int*>(&sDisplayMode), static_cast<int>(eDisplayMode::LocalIndependent));
		if (sDisplayMode == eDisplayMode::LocalMirror)
		{
			ImGui::TextWrapped("(Note: While the content is duplicated on local screen, it can only be interacted with on the netImgui remote server.)");
		}
	}
}

void Client_Draw_SharedContent()
{
	ImGui::NewLine();
	ImGui::Text("Elapsed time : %02i:%02i", static_cast<int>(ImGui::GetTime())/60, static_cast<int>(ImGui::GetTime())%60);
	ImGui::Text("Delta time   : %07.04fs", ImGui::GetIO().DeltaTime );
	ImGui::Text("Drawing      : %s", NetImgui::IsDrawingRemote() ? "Remotely" : "Locally" );
}

//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
ImDrawData* Client_Draw()
{	
	// Saving local window size
	ImVec2 CurrentWindowSize = ImGui::GetIO().DisplaySize;

	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame
	//---------------------------------------------------------------------------------------------
	//	IMPORTANT NOTE: NetImgui uses the curent context when 'ConnectToApp' or 'ConnectFromApp' 
	//	was called.
	//
	//	For this sample, main content is always displayed on ContextMain, both when 
	//	connected to server or only drawing locally. We then have a secondary context to display
	//	locally, when we are drawing the main content for the remote connection.
	//
	//	You could decide to have a dedicated Remote NetImgui context instead. 
	//---------------------------------------------------------------------------------------------

	// Because a window resize might happen while main context wasn't the default, we are
	// making sure it knows about the current size. While we are connected remotely it doesn't 
	// matter, but once disconnected, we want the context to be up to date on the real window size.
	// This is not needed if your own engine already update the display size every frame or this
	// is the context that always receives the window size updates.
	ImGui::SetCurrentContext(gpContextMain);
	ImGui::GetIO().DisplaySize = CurrentWindowSize; 
	if( NetImgui::NewFrame(true) )
	{
		//-----------------------------------------------------------------------------------------
		// (2) Draw ImGui Content 		
		//-----------------------------------------------------------------------------------------
		// This is the main content that we are always displaying. When connected, it will appear
		// on the remote server, otherwise will appear in the local window
		ClientUtil_ImGuiContent_Common("SampleDualUI");
		ImGui::SetNextWindowPos(ImVec2(32,48), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(400,400), ImGuiCond_Once);
		if (ImGui::Begin("SampleDualUI", nullptr))
		{	
			Client_Draw_MainContent();
			Client_Draw_SharedContent();			
			Client_Draw_LocalContent();
			Client_Draw_RemoteContent();
			ImGui::End();
		}

		//-----------------------------------------------------------------------------------------
		// (3) Finish the frame, preparing the drawing data and...
		// (3a) Send the data to the netImGui server when connected
		//-----------------------------------------------------------------------------------------
		NetImgui::EndFrame();
	}
	
	//---------------------------------------------------------------------------------------------
	// (4) Draw a second UI for local display, when connected and display mode is 'Independent'
	//---------------------------------------------------------------------------------------------
	if( NetImgui::IsConnected() && sDisplayMode == eDisplayMode::LocalIndependent )
	{
		ImGui::SetCurrentContext(gpContextSide);
		// Need to let this context know about the window size, when we just activated it
		ImGui::GetIO().DisplaySize = CurrentWindowSize;	
		ImGui::NewFrame();
		ImGui::SetNextWindowSize(ImVec2(300, 220), ImGuiCond_Once);
		if( ImGui::Begin("Extra Local Window") )
		{
			ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Local Window");
			ImGui::TextWrapped("Demonstrate a window content displayed locally while we have some distinct content displayed remotely.");
			Client_Draw_SharedContent();
			Client_Draw_LocalContent();
			// Demonstration: Even though we are calling this, we are testing where the output is meant to be,
			// inside the function, and will never end up be drawn here
			Client_Draw_RemoteContent();	
		}
		ImGui::End();
		ImGui::Render();

		// Note: When this mode is active, the Side Context is left as current active context in 
		//		 Dear ImGui. This will let the Dear ImGui backend forward all input/display size
		//		 to this context instead of the MainContext which the remote is using.
	}

	return NetImgui::IsConnected() && sDisplayMode == eDisplayMode::LocalNone ? nullptr : ImGui::GetDrawData();
} 

} // namespace SampleClient
