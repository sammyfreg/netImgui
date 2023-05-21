//=================================================================================================
// SAMPLE TEXTURES
//-------------------------------------------------------------------------------------------------
// Example of using various textures in the ImGui context, after making netImgui aware of them.
//=================================================================================================

#include <NetImgui_Api.h>
#include <array>
#include "..\Common\Sample.h"

// Methods declared in main.cpp, extern declare to avoid having to include 'd3d11.h' here
extern void TextureCreate(const uint8_t* pPixelData, uint32_t width, uint32_t height, void*& pTextureViewOut);
extern void TextureDestroy(void*& pTextureView);

namespace SampleClient
{
static void*	gDefaultEmptyTexture = nullptr;
static void*	gCustomTextureView[static_cast<int>(NetImgui::eTexFormat::kTexFmt_Count)];
const char*		gTextureFormatName[static_cast<int>(NetImgui::eTexFormat::kTexFmt_Count)] = { "R8", "RGBA8" };

//=================================================================================================
// Add a new texture of a specified format, to our sample. Will be displayed locally or remotely
//=================================================================================================
void CustomTextureCreate(NetImgui::eTexFormat eTexFmt)
{
	constexpr uint32_t BytePerPixelMax = 4;
	constexpr uint32_t Width = 8;
	constexpr uint32_t Height = 8;
	uint8_t pixelData[Width * Height * BytePerPixelMax];

	switch (eTexFmt)
	{
	case NetImgui::eTexFormat::kTexFmtA8:
		for (uint8_t y(0); y < Height; ++y) {
			for (uint8_t x(0); x < Width; ++x)
			{
				pixelData[(y * Width + x)] = 0xFF * x / 8u;
			}
		}break;
	case NetImgui::eTexFormat::kTexFmtRGBA8:
		for (uint8_t y(0); y < Height; ++y) {
			for (uint8_t x(0); x < Width; ++x)
			{
				pixelData[(y * Width + x) * 4 + 0] = 0xFF * x / 8u;
				pixelData[(y * Width + x) * 4 + 1] = 0xFF * y / 8u;
				pixelData[(y * Width + x) * 4 + 2] = 0xFF;
				pixelData[(y * Width + x) * 4 + 3] = 0xFF;
			}
		}break;
	case NetImgui::eTexFormat::kTexFmtCustom:
	case NetImgui::eTexFormat::kTexFmt_Invalid: assert(0); break;
	}
	TextureCreate(pixelData, Width, Height, gCustomTextureView[static_cast<int>(eTexFmt)]);													// For local display
	NetImgui::SendDataTexture(static_cast<ImTextureID>(gCustomTextureView[static_cast<int>(eTexFmt)]), pixelData, Width, Height, eTexFmt);	// For remote display
}

//=================================================================================================
//
//=================================================================================================
void CustomTextureDestroy(NetImgui::eTexFormat eTexFmt)
{
	if (gCustomTextureView[static_cast<int>(eTexFmt)])
	{
		NetImgui::SendDataTexture(static_cast<ImTextureID>(gCustomTextureView[static_cast<int>(eTexFmt)]), nullptr, 0, 0, NetImgui::eTexFormat::kTexFmt_Invalid);
		TextureDestroy(gCustomTextureView[static_cast<int>(eTexFmt)]);
	}
}

//=================================================================================================
//
//=================================================================================================
bool Client_Startup()
{
	if (!NetImgui::Startup())
		return false;

	std::array<uint8_t, 8*8*4> pixelData;
	pixelData.fill(0);

	TextureCreate(pixelData.data(), 8, 8, gDefaultEmptyTexture);
	NetImgui::SendDataTexture(static_cast<ImTextureID>(gDefaultEmptyTexture), pixelData.data(), 8, 8, NetImgui::eTexFormat::kTexFmtRGBA8);

	// Can have more ImGui initialization here, like loading extra fonts.
	// ...

	return true;
}

//=================================================================================================
//
//=================================================================================================
void Client_Shutdown()
{
	NetImgui::SendDataTexture(static_cast<ImTextureID>(gDefaultEmptyTexture), nullptr, 0, 0, NetImgui::eTexFormat::kTexFmt_Invalid);
	TextureDestroy(gDefaultEmptyTexture);
	NetImgui::Shutdown();
}

//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
ImDrawData* Client_Draw()
{
	bool bCanDisplayLocally(false);

	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame
	//---------------------------------------------------------------------------------------------
	if (NetImgui::NewFrame(true))
	{		
		//-----------------------------------------------------------------------------------------
		// (2) Draw ImGui Content 		
		//-----------------------------------------------------------------------------------------
		ClientUtil_ImGuiContent_Common("SampleTextures");
		ImGui::SetNextWindowPos(ImVec2(32, 48), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);
		if (ImGui::Begin("Sample Textures", nullptr))
		{
			const ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			const ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);

			ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Demonstration of textures usage with netImgui.");	
			ImGui::TextWrapped("Note: Textures properly displayed on netImgui server only. Original 'Dear ImGui' sample code was left intact for easier maintenance and is limited to the RGBA format. This is not a NetImgui limitation.");
			ImGui::NewLine();
			for (int i = 0; i < static_cast<int>(NetImgui::eTexFormat::kTexFmt_Count); ++i)
			{
				// Only display this on remote connection (associated texture note created locally)
				ImGui::PushID(i);				
				if (!gCustomTextureView[i] && ImGui::Button("Texture Create", ImVec2(140,0)))
					CustomTextureCreate(static_cast<NetImgui::eTexFormat>(i));
				else if (gCustomTextureView[i] && ImGui::Button("Texture Destroy", ImVec2(140,0)))
					CustomTextureDestroy(static_cast<NetImgui::eTexFormat>(i));
				ImGui::SameLine();
				ImGui::Text("(%s)", gTextureFormatName[i]);	

				// Note: Test drawing with invalid texture (making sure netImgui App handles it) 
				// Avoid drawing with invalid texture if will be displayed locally (not handled by 'ImGui_ImplDX11_RenderDrawData' example code)				
				{					
					ImGui::SameLine(220.f);
					void* pTexture = gCustomTextureView[i] ? gCustomTextureView[i] : gDefaultEmptyTexture;
					ImGui::Image(reinterpret_cast<ImTextureID>(pTexture), ImVec2(128, 64), ImVec2(0, 0), ImVec2(1, 1), tint_col, border_col);
				}
				
				ImGui::PopID();
			}
			
			if( NetImgui::IsDrawingRemote() )
			{
				ImGui::NewLine();
				ImGui::TextWrapped("Note: Invalid textures handled by netImgui server by using a default invisible texture.");
				ImGui::NewLine();
				ImGui::TextUnformatted("Invalid texture");
				ImGui::SameLine(220.f);
				ImGui::Image(reinterpret_cast<ImTextureID>(0x12345678), ImVec2(128, 64), ImVec2(0, 0), ImVec2(1, 1), tint_col, border_col);
				// Invalid textures not handled by the base Dear ImGui Sample code, but netImgui server can.
				// Making sure we are not trying to draw an invalid texture locally, after a disconnect from remote server
				bCanDisplayLocally = false;	
			}
			else
			{
				bCanDisplayLocally = true;
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
	return bCanDisplayLocally ? ImGui::GetDrawData() : nullptr;
}

} // namespace SampleClient

