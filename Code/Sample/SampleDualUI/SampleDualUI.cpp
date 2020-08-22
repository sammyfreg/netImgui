//=================================================================================================
// SAMPLE DUAL UI
//-------------------------------------------------------------------------------------------------
// Example of supporting 2 differents ImGui display at the same time. One on the remote server
// and the second in the local client.
//=================================================================================================

#include <NetImgui_Api.h>
#include "..\Common\WarningDisable.h"
#include "..\Common\Sample.h"

namespace SampleClient
{
enum class eDisplayMode : int { LocalNone, LocalMirror, LocalIndependent };
static eDisplayMode skDisplayMode = eDisplayMode::LocalNone;

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
		ImGui::RadioButton("None", reinterpret_cast<int*>(&skDisplayMode), static_cast<int>(eDisplayMode::LocalNone)); ImGui::SameLine();
		ImGui::RadioButton("Mirror", reinterpret_cast<int*>(&skDisplayMode), static_cast<int>(eDisplayMode::LocalMirror)); ImGui::SameLine();
		ImGui::RadioButton("Independent", reinterpret_cast<int*>(&skDisplayMode), static_cast<int>(eDisplayMode::LocalIndependent));
		if (skDisplayMode == eDisplayMode::LocalMirror)
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
const ImDrawData* Client_Draw()
{
	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame
	//---------------------------------------------------------------------------------------------
	if( NetImgui::NewFrame(true) )
	{
		//-----------------------------------------------------------------------------------------
		// (2) Draw ImGui Content 		
		//-----------------------------------------------------------------------------------------
		// IMPORTANT NOTE: We are telling the connection that we want the original Context 'Cloned'.
		// Meaning a duplicate context will be created for remote drawing. This leaves the original 
		// Context untouched and free to continue using for a separate ImGui output in the 
		// 'Local Independent' UI drawing section. 'Cloning' the context is not needed if you are 
		// not planning to keep drawing in local windows while connected, or you can manage this 
		// yourself using your own new context.
		ClientUtil_ImGuiContent_Common("SampleDualUI", true);

		if (ImGui::Begin("SampleDualUI", nullptr))
		{	
			Client_Draw_MainContent();
			Client_Draw_SharedContent();			
			Client_Draw_LocalContent();
			Client_Draw_RemoteContent();
			ImGui::End();
		}

		//---------------------------------------------------------------------------------------------
		// (3) Finish the frame, preparing the drawing data and...
		// (3a) Send the data to the netImGui server when connected
		//---------------------------------------------------------------------------------------------
		NetImgui::EndFrame();
	}
	
	//---------------------------------------------------------------------------------------------
	// (4) Draw a second UI for local display, when connected and display mode is 'Independent'
	//---------------------------------------------------------------------------------------------
	if( NetImgui::IsConnected() && skDisplayMode == eDisplayMode::LocalIndependent )
	{
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
	}

	//---------------------------------------------------------------------------------------------
	// (5) Decide what to render locally
	//---------------------------------------------------------------------------------------------
	if( NetImgui::IsConnected() )
	{ 
		switch( skDisplayMode)
		{
		case eDisplayMode::LocalNone:			return nullptr;
		case eDisplayMode::LocalMirror:			return NetImgui::GetDrawData();
		case eDisplayMode::LocalIndependent:	return ImGui::GetDrawData();
		}
	}

	// When disconnected, NetImgui::GetDrawData() and ImGui::GetDrawData() are the same
	return NetImgui::GetDrawData();
} 

} // namespace SampleClient
