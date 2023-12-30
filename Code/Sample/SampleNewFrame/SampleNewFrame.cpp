//=================================================================================================
// SAMPLE NEW FRAME
//-------------------------------------------------------------------------------------------------
// Example of handling frame skipping. 
// When connected to NetImgui Server, we don't need to refresh the ImGui content every frame.
// If your program can handle skipping drawing, saves CPU cycles when no refresh is expected.
//=================================================================================================

#include <NetImgui_Api.h>
#include "../Common/Sample.h"

//=================================================================================================
// SAMPLE CLASS
//=================================================================================================
class SampleNewFrame : public SampleClient_Base
{
public:
						SampleNewFrame() : SampleClient_Base("SampleNewFrame") {}
	virtual ImDrawData* Draw() override;

protected:
	enum class eDrawUpdateMode : int { Always, OnDemand };
	void				Draw_Content();
	ImDrawData*			Draw_ModeOnDemand();
	ImDrawData*			Draw_ModeAlways();
	
	eDrawUpdateMode		mFrameRefreshMode	= eDrawUpdateMode::Always;
	uint32_t			mFrameDrawnCount	= 0;
	uint32_t			mFrameSkippedCount	= 0;
};

//=================================================================================================
// GET SAMPLE
// Each project must return a valid sample object
//=================================================================================================
SampleClient_Base& GetSample()
{
	static SampleNewFrame sample;
	return sample;
}

//=================================================================================================
// Imgui content 
//=================================================================================================
void SampleNewFrame::Draw_Content()
{
	bool bModeChanged(false);

	SampleClient_Base::Draw_Connect(); //Note: Connection to remote server done in there

	ImGui::SetNextWindowPos(ImVec2(32,48), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(700,450), ImGuiCond_Once);
	if (ImGui::Begin("Sample NewFrame", nullptr))
	{
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Demonstration of frame skip handling.");
		ImGui::TextWrapped(	"Demonstrate how to reduce CPU usage by only redrawing ImGui content when needed. "
							"The netImgui remote server does not expect a UI refresh every frame, we can skip some redraw. "
							"When a codebase does not support skipping a frame, internally we just assign a placeholder "
							"ImGui context and discard the result.");

		ImGui::NewLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Frame Refresh Mode :");		
		ImGui::SameLine();	bModeChanged |= ImGui::RadioButton("Always", reinterpret_cast<int*>(&mFrameRefreshMode), static_cast<int>(eDrawUpdateMode::Always));
		ImGui::SameLine();	bModeChanged |= ImGui::RadioButton("On Demand", reinterpret_cast<int*>(&mFrameRefreshMode), static_cast<int>(eDrawUpdateMode::OnDemand));
		if( bModeChanged )
			mFrameDrawnCount = mFrameSkippedCount = 0;
		
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f,0.75f,0.75f,1.f));
		ImGui::TextWrapped( NetImgui::IsDrawingRemote()	? "Note: The ratio of frames skipped depends on the 'Refresh Rate' configured in the Server's settings." 
														: "Note: Frame skipping only happen when drawing for remote connection.");
		ImGui::PopStyleColor();
	
		ImGui::NewLine();
		ImGui::Text("Draw Count: %i", mFrameDrawnCount);
		ImGui::Text("Skip Count: %i", mFrameSkippedCount);
		ImGui::Text("Time Saved: %i%%", mFrameSkippedCount ? mFrameSkippedCount * 100 / (mFrameDrawnCount+mFrameSkippedCount) : 0);
	}
	ImGui::End();
}

//=================================================================================================
// Without Frame skip support. User redraw content every frame, even when netImgui doesn't
// require a new drawframe
//=================================================================================================
ImDrawData* SampleNewFrame::Draw_ModeAlways()
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
	Draw_Content();

	//---------------------------------------------------------------------------------------------
	// (3) Finish the frame, preparing the drawing data and...
	// (3a) Send the data to the netImGui server when connected
	//---------------------------------------------------------------------------------------------
	NetImgui::EndFrame();
	mFrameDrawnCount++;

	//---------------------------------------------------------------------------------------------
	// (4) Forward to drawing data our local renderer when not connected or 
	//	connected and wanting to mirror the remote content. 
	//---------------------------------------------------------------------------------------------
	return !NetImgui::IsConnected() ? ImGui::GetDrawData() : nullptr;
}

//=============================================================================================
// With Frame skip support
// This code path handle not rendering a frame when not needed. netImgui doesn't require 
// redrawing every frame.
//=============================================================================================
ImDrawData* SampleNewFrame::Draw_ModeOnDemand()
{
	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame
	// When 'NewFrame' returns false, we should skip drawing this frame
	// Note 1 : Key point is using the 'NewFrame()' return value to decide when to skip drawing
	// Note 2 : Instead of relying on 'NewFrame()' return value, 'IsDrawing()' can be queried 
	// Note 3 : Frame skipping only happens when remote drawing
	// Note 4 : To enable this, we explicitly call NetImgui::NewFrame/EndFrame() instead of
	//			relying on ImGui::NewFrame/Render interception by NetImgui.
	//---------------------------------------------------------------------------------------------
	if (NetImgui::NewFrame(true))
	{		
		//-----------------------------------------------------------------------------------------
		// (2) Draw ImGui Content
		//-----------------------------------------------------------------------------------------		
		Draw_Content();

		//---------------------------------------------------------------------------------------------
		// (3) Finish the frame, preparing the drawing data and...
		// (3a) Send the data to the netImGui server when connected
		//---------------------------------------------------------------------------------------------
		NetImgui::EndFrame();

		//---------------------------------------------------------------------------------------------
		// (4) Forward to drawing data our local renderer when not connected or 
		//	connected and wanting to mirror the remote content. 
		//---------------------------------------------------------------------------------------------
		mFrameDrawnCount++;
		return !NetImgui::IsConnected() ? ImGui::GetDrawData() : nullptr;
	}
		
	mFrameSkippedCount++;
	return nullptr;
}


//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
ImDrawData* SampleNewFrame::Draw()
{	
	switch( mFrameRefreshMode )
	{
	case eDrawUpdateMode::Always:	return Draw_ModeAlways();
	case eDrawUpdateMode::OnDemand:	return Draw_ModeOnDemand();
	}
	return nullptr;
}

