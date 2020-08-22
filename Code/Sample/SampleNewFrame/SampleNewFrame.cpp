//=================================================================================================
// SAMPLE DUAL UI
//-------------------------------------------------------------------------------------------------
// Example of supporting 2 differents ImGui display at the same time. One on the remote server
// and the second in the local client.
//=================================================================================================

#include <NetImgui_Api.h>
#include <array>
#include "..\Common\Sample.h"
#include "..\Common\WarningDisable.h"

namespace SampleClient
{	

enum class eDrawUpdateMode : int { Always, OnDemand };
static eDrawUpdateMode sDrawUpdateMode	= eDrawUpdateMode::Always;
static uint32_t sFrameDrawnCount		= 0;
static uint32_t sFrameSkippedCount		= 0;

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
// Imgui content 
//=================================================================================================
void Client_Draw_Content()
{
	bool bModeChanged(false);

	ClientUtil_ImGuiContent_Common("SampleNewFrame", false);

	if (ImGui::Begin("Sample NewFrame", nullptr))
	{
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Demonstration of frame skip handling.");
		ImGui::TextWrapped(	"Demonstrate how to reduce CPU usage by only redrawing ImGui content when needed. "
							"The netImgui remote server does not expect a UI refresh every frame, we can skip some redraw. "
							"When a codebase does not support skipping a frame, internally we just assign a placeholder "
							"ImGui context and discard the result.");

		ImGui::NewLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Draw Update Mode :");		
		ImGui::SameLine();	bModeChanged |= ImGui::RadioButton("Always", reinterpret_cast<int*>(&sDrawUpdateMode), static_cast<int>(eDrawUpdateMode::Always));
		ImGui::SameLine();	bModeChanged |= ImGui::RadioButton("On Demand", reinterpret_cast<int*>(&sDrawUpdateMode), static_cast<int>(eDrawUpdateMode::OnDemand));
		if( bModeChanged )
			sFrameDrawnCount = sFrameSkippedCount = 0;
		
		if( !NetImgui::IsDrawingRemote() )
		{
			ImGui::TextUnformatted("(Note: There is not frame skipping when not remote drawing.)");
		}
	
		ImGui::NewLine();
		ImGui::Text("Draw Count: %i", sFrameDrawnCount);
		ImGui::Text("Skip Count: %i", sFrameSkippedCount);
		ImGui::Text("Time Saved: %i%%", sFrameSkippedCount ? sFrameSkippedCount * 100 / (sFrameDrawnCount+sFrameSkippedCount) : 0);
	}
	ImGui::End();
}

//=================================================================================================
// Without Frame skip support. User redraw content every frame, even when netImgui doesn't
// require a new drawframe
//=================================================================================================
const ImDrawData* Client_Draw_ModeAlways()
{
	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame
	// If your codebase doesn't handle skipping drawing the menu for some frame 
	// (either by not calling your Imgui
	//---------------------------------------------------------------------------------------------
	NetImgui::NewFrame(false);

	//-----------------------------------------------------------------------------------------
	// (2) Draw ImGui Content 		
	//-----------------------------------------------------------------------------------------
	Client_Draw_Content();

	//---------------------------------------------------------------------------------------------
	// (3) Finish the frame, preparing the drawing data and...
	// (3a) Send the data to the netImGui server when connected
	//---------------------------------------------------------------------------------------------
	NetImgui::EndFrame();
	sFrameDrawnCount++;

	//---------------------------------------------------------------------------------------------
	// (4) Forward to drawing data our local renderer when not connected or 
	//	connected and wanting to mirror the remote content. 
	//---------------------------------------------------------------------------------------------
	return !NetImgui::IsConnected() ? NetImgui::GetDrawData() : nullptr;
}

//=============================================================================================
// With Frame skip support
// This code path handle not rendering a frame when not needed. netImgui doesn't require 
// redrawing every frame.
//=============================================================================================
const ImDrawData* Client_Draw_ModeOnDemand()
{
	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame
	// When 'NewFrame' returns false, we should skip drawing this frame
	// Note 1 : Key point is using 'true' in 'NewFrame()', letting system know to skip empty frames
	// Note 2 : Frame skipping only happens when remote drawing
	// Note 3 : Instead of relying on 'NewFrame()' return value of 'NewFrame', can use 'IsDrawing()'
	//---------------------------------------------------------------------------------------------
	if (NetImgui::NewFrame(true))
	{		
		//-----------------------------------------------------------------------------------------
		// (2) Draw ImGui Content 		
		//-----------------------------------------------------------------------------------------		
		Client_Draw_Content();

		//---------------------------------------------------------------------------------------------
		// (3) Finish the frame, preparing the drawing data and...
		// (3a) Send the data to the netImGui server when connected
		//---------------------------------------------------------------------------------------------
		NetImgui::EndFrame();

		//---------------------------------------------------------------------------------------------
		// (4) Forward to drawing data our local renderer when not connected or 
		//	connected and wanting to mirror the remote content. 
		//---------------------------------------------------------------------------------------------
		sFrameDrawnCount++;
		return !NetImgui::IsConnected() ? NetImgui::GetDrawData() : nullptr;
	}
		
	sFrameSkippedCount++;
	return nullptr;
}


//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
const ImDrawData* Client_Draw()
{	
	switch( sDrawUpdateMode )
	{
	case eDrawUpdateMode::Always:	return Client_Draw_ModeAlways();
	case eDrawUpdateMode::OnDemand:	return Client_Draw_ModeOnDemand();
	}
	return nullptr;
}
} // namespace SampleClient
