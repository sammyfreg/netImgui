//=================================================================================================
// SAMPLE INDEX 16/32Bits
//-------------------------------------------------------------------------------------------------
// Sample testing render of large amount of shapes, pushing the vertices count past the 65k
// limit of 16bits indices.
// 
// Note: This source file is compiled under 2 samples projects, one with Dear ImGui using 
//		 16bits indices and the other one using 32 bits indices.
// 
// Note: This sample is meant as a UnitTest ensuring large draws amount is handled properly on 
//		 NetImgui Server application
//=================================================================================================

#include <NetImgui_Api.h>
#include <array>
#include "../Common/Sample.h"

//=================================================================================================
// SAMPLE CLASS
//=================================================================================================
class SampleIndex : public Sample::Base
{
public:
					SampleIndex() : Base(sizeof(ImDrawIdx) == 2 ? "SampleIndex16Bits" : "SampleIndex32Bits") {}
virtual ImDrawData* Draw() override;
};

//=================================================================================================
// GET SAMPLE
// Each project must return a valid sample object
//=================================================================================================
Sample::Base& GetSample()
{
	static SampleIndex sample;
	return sample;
}

//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
ImDrawData* SampleIndex::Draw()
{
	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame
	//---------------------------------------------------------------------------------------------
	if (NetImgui::NewFrame(true))
	{		
		//-----------------------------------------------------------------------------------------
		// (2) Draw ImGui Content 		
		//-----------------------------------------------------------------------------------------
		Base::Draw_Connect(); //Note: Connection to remote server done in there

		ImGui::SetNextWindowPos(ImVec2(32, 48), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(525, 820), ImGuiCond_Once);
		if (ImGui::Begin("Sample Index", nullptr))
		{
			ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Large amount of mesh drawing.");
			ImGui::TextWrapped("This sample is meant as a UnitTest ensuring large draws amount is handled properly on NetImgui Server application");
			ImGui::NewLine();

			static size_t sIndexSize =  sizeof(ImDrawIdx); // Using a static value to prevent VS warning :/ (C4127: Conditional Expression is Constant)
			if( sIndexSize == 2 ){
				ImGui::TextWrapped("Note: This sample uses Dear ImGui compiled with 16bits indices. Meaning that each draw will be splitted in multiple drawcalls of less than 65k vertices each.");
			}
			else {
				ImGui::TextWrapped("Note: This sample uses Dear ImGui compiled with 32bits indices. Meaning that each draw can be sent as a single drawcall, even when having more than 65k vertices.");
			}
			
			ImGui::NewLine();

			ImDrawList* draw_list	= ImGui::GetWindowDrawList();
			const ImVec2 winPos		= ImGui::GetCursorScreenPos();
			constexpr float th		= 3.f;//(n == 0) ? 1.0f : thickness;
			constexpr int segments	= 64;
			constexpr float size	= 16.f;
			constexpr float spacing = 4.f;
			for (float y(0); y < 25.f; y++) {
				for (float x(0); x < 25.f; x++) {
					float posX = winPos.x + (x + 0.5f)*(size + spacing);
					float posY = winPos.y + (y + 0.5f)*(size + spacing);
					draw_list->AddCircle(ImVec2(posX, posY), size*0.5f, ImColor(1.f,1.f,1.f,1.f), segments, th);
				}
			}
		}
		ImGui::End();


		//-----------------------------------------------------------------------------------------
		// (3) Finish the frame, preparing the drawing data and...
		// (3a) Send the data to the netImGui server when connected
		//-----------------------------------------------------------------------------------------
		NetImgui::EndFrame();
	}

	//---------------------------------------------------------------------------------------------
	// (4b) Render nothing locally (when connected)
	//---------------------------------------------------------------------------------------------
	return !NetImgui::IsConnected() ? ImGui::GetDrawData() : nullptr;
}
