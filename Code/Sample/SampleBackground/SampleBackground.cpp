//=================================================================================================
// SAMPLE BACKGROUND
//-------------------------------------------------------------------------------------------------
// Example of using the NetImgui 'Background' setting
//=================================================================================================

#include <NetImgui_Api.h>
#include <array>
#include "..\Common\Sample.h"

// Methods declared in main.cpp, extern declare to avoid having to include 'd3d11.h' here
extern void TextureCreate(const uint8_t* pPixelData, uint32_t width, uint32_t height, void*& pTextureViewOut);
extern void TextureDestroy(void*& pTextureView);

namespace SampleClient
{

static void*	gTextureView;

//=================================================================================================
//
//=================================================================================================
bool Client_Startup()
{
	if (!NetImgui::Startup())
		return false;

	
	constexpr uint16_t kSize	= 256;
	constexpr float kFadeSize	= 16.f;
	uint32_t pixels[kSize][kSize];
	for (uint32_t y(0); y < kSize; ++y) {
		for (uint32_t x(0); x < kSize; ++x) 
		{
			float offsetX	= static_cast<float>(x) - static_cast<float>(kSize/2);
			float offsetY	= static_cast<float>(y) - static_cast<float>(kSize/2);
			float radius	= static_cast<float>(sqrt(offsetX*offsetX+offsetY*offsetY));
			float alpha		= 1.f - std::min(1.f, std::max(0.f, (radius - (static_cast<float>(kSize/2)-kFadeSize))) / kFadeSize);
			pixels[y][x]	= ImColor( static_cast<uint8_t>(x), static_cast<uint8_t>(y), (x+y) <= 255 ? 0 : 255-(x+y), static_cast<uint8_t>(255.f*alpha));
		}
	}
	TextureCreate(reinterpret_cast<uint8_t*>(pixels), kSize, kSize, gTextureView);													// For local display
	NetImgui::SendDataTexture(static_cast<ImTextureID>(gTextureView), pixels, kSize, kSize, NetImgui::eTexFormat::kTexFmtRGBA8);	// For remote display

	return true;
}

//=================================================================================================
//
//=================================================================================================
void Client_Shutdown()
{
	TextureDestroy(gTextureView);
	NetImgui::Shutdown();
}

//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
ImDrawData* Client_Draw()
{
	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame
	//---------------------------------------------------------------------------------------------
	if (NetImgui::NewFrame(true))
	{		
		//-----------------------------------------------------------------------------------------
		// (2) Draw ImGui Content 		
		//-----------------------------------------------------------------------------------------
		ClientUtil_ImGuiContent_Common("SampleBackground");
		ImGui::SetNextWindowPos(ImVec2(32, 48), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);
		if (ImGui::Begin("Sample Background", nullptr))
		{
			ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Demonstration of NetImgui's Background settings.");	
			static ImVec4 sBgColor(0.2f,0.2f,0.2f,1.f);
			static ImVec4 sTextureTint(1,1,1,0.5f);
			static bool sUseTextureOverride(false);

			ImGui::ColorEdit4("Background", reinterpret_cast<float*>(&sBgColor), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_PickerHueWheel);
			ImGui::ColorEdit4("Logo Tint", reinterpret_cast<float*>(&sTextureTint), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_PickerHueWheel);			
			ImGui::Checkbox("Replace Background Texture", &sUseTextureOverride);
			ImGui::Image(gTextureView, ImVec2(64,64));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
			ImGui::TextWrapped("(Note: Custom background settings only applied on remote server)");
			ImGui::PopStyleColor();
			if( sUseTextureOverride )
			{
				NetImgui::SetBackground(sBgColor, sTextureTint, static_cast<ImTextureID>(gTextureView));
			}
			else
			{ 
				NetImgui::SetBackground(sBgColor, sTextureTint);
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

} // namespace SampleClient

