//=================================================================================================
// SAMPLE TEXTURES
//-------------------------------------------------------------------------------------------------
// Example of using various textures in the ImGui context, after making netImgui aware of them.
//=================================================================================================

#include <NetImgui_Api.h>
#include <array>
#include "../Common/Sample.h"

// Enable handling of a Custom Texture Format samples. 
// Only meant as an example, users are free to replace it with their own handling of
// custom texture formats. Look for this define for implementation details.
#define TEXTURE_CUSTOM_SAMPLE 1

// Methods declared in main.cpp, extern declare to avoid having to include 'd3d11.h' here
extern void TextureCreate(const uint8_t* pPixelData, uint32_t width, uint32_t height, void*& pTextureViewOut);
extern void TextureDestroy(void*& pTextureView);

//=================================================================================================
// SAMPLE CLASS
//=================================================================================================
class SampleTextures : public Sample::Base
{
public:
						SampleTextures() : Base("SampleTextures") {}
	virtual bool		Startup() override;
	virtual void		Shutdown() override;
	virtual ImDrawData* Draw() override;
protected:
	void				TextureFormatCreation(NetImgui::eTexFormat eTexFmt);
	void				TextureFormatDestruction(NetImgui::eTexFormat eTexFmt);
	void*				mDefaultEmptyTexture = nullptr;
	void*				mCustomTextureView[static_cast<int>(NetImgui::eTexFormat::kTexFmt_Count)];
};

//=================================================================================================
// GET SAMPLE
// Each project must return a valid sample object
//=================================================================================================
Sample::Base& GetSample()
{
	static SampleTextures sample;
	return sample;
}

#if TEXTURE_CUSTOM_SAMPLE
//=================================================================================================
// This is our Custom texture data format
// It can be customized to anything needed, as long as Client/Server use the same content
// For this sample, we rely on the struct size and a starting stamp to identify the custom data
// 
// You can imagine using custom texture format to send some compressed video frame, 
// or other new format handling, but require modifications to the Server codebase 
struct customTextureData1{ 
	static constexpr uint64_t kStamp = 0x0123456789000001;
	uint64_t m_Stamp		= kStamp;
	uint32_t m_ColorStart	= 0; 
	uint32_t m_ColorEnd		= 0; 
};

struct customTextureData2{
	static constexpr uint64_t kStamp = 0x0123456789000002;
	uint64_t m_Stamp		= kStamp;
	uint64_t m_TimeUS		= 0;
};
//=================================================================================================
#endif


//=================================================================================================
// Add a new texture of a specified format, to our sample. Will be displayed locally or remotely
//=================================================================================================
void SampleTextures::TextureFormatCreation(NetImgui::eTexFormat eTexFmt)
{
//=================================================================================================
// Note: Example of user adding their own custom texture format
//		 It needs to be handled by adding the code in NetImguiServer 
//		 inside the 'ProcessTexture_Custom' function.
//		 If user change the Server function, then this sample code won't 
//		 be compatible anymore. 
#if TEXTURE_CUSTOM_SAMPLE
	if (eTexFmt == NetImgui::eTexFormat::kTexFmtCustom) {
		customTextureData1 customTextrueData;
		customTextrueData.m_ColorStart	= ImColor(0.2f,0.2f,1.f,1.f);
		customTextrueData.m_ColorEnd	= ImColor(1.f,0.2f,0.2f,1.f);
		constexpr uint32_t Width		= 8;
		constexpr uint32_t Height		= 8;
		uint8_t pixelData[Width * Height * 4]={};
		TextureCreate(pixelData, Width, Height, mCustomTextureView[static_cast<int>(eTexFmt)]);	// For local display
		NetImgui::SendDataTexture(NetImgui::Internal::TextureCastFromPtr(mCustomTextureView[static_cast<int>(eTexFmt)]), &customTextrueData, Width, Height, eTexFmt, sizeof(customTextureData1));	// For remote display
	}
	else
#endif
//=================================================================================================
// 
	//----------------------------------------------------------------------------
	// Normal Texture Handling (of normal supported formats)
	//----------------------------------------------------------------------------
	{
		constexpr uint32_t BytePerPixelMax	= 4;
		constexpr uint32_t Width			= 8;
		constexpr uint32_t Height			= 8;
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
		TextureCreate(pixelData, Width, Height, mCustomTextureView[static_cast<int>(eTexFmt)]);													// For local display
		NetImgui::SendDataTexture(NetImgui::Internal::TextureCastFromPtr(mCustomTextureView[static_cast<int>(eTexFmt)]), pixelData, Width, Height, eTexFmt);	// For remote display
	}
}

//=================================================================================================
//
//=================================================================================================
void SampleTextures::TextureFormatDestruction(NetImgui::eTexFormat eTexFmt)
{
	if (mCustomTextureView[static_cast<int>(eTexFmt)])
	{
		// Remove texture from the remote client (on NetImgui Server)
		NetImgui::SendDataTexture(NetImgui::Internal::TextureCastFromPtr(mCustomTextureView[static_cast<int>(eTexFmt)]), nullptr, 0, 0, NetImgui::eTexFormat::kTexFmt_Invalid);
		
		// Remove texture created locally (in this sample application)
		TextureDestroy(mCustomTextureView[static_cast<int>(eTexFmt)]);
	}
}

//=================================================================================================
//
//=================================================================================================
bool SampleTextures::Startup()
{
	if (!Base::Startup())
		return false;

	std::array<uint8_t, 8*8*4> pixelData;
	pixelData.fill(0);

	TextureCreate(pixelData.data(), 8, 8, mDefaultEmptyTexture);
	NetImgui::SendDataTexture(NetImgui::Internal::TextureCastFromPtr(mDefaultEmptyTexture), pixelData.data(), 8, 8, NetImgui::eTexFormat::kTexFmtRGBA8);

	return true;
}

//=================================================================================================
//
//=================================================================================================
void SampleTextures::Shutdown()
{
	NetImgui::SendDataTexture(NetImgui::Internal::TextureCastFromPtr(mDefaultEmptyTexture), nullptr, 0, 0, NetImgui::eTexFormat::kTexFmt_Invalid);
	TextureDestroy(mDefaultEmptyTexture);
	Base::Shutdown();
}

//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
ImDrawData* SampleTextures::Draw()
{
	const char*	gTextureFormatName[static_cast<int>(NetImgui::eTexFormat::kTexFmt_Count)] = { "R8", "RGBA8", "Custom"};
	bool bCanDisplayLocally(false);

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
		ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_Once);
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
				if (!mCustomTextureView[i] && ImGui::Button("Texture Create", ImVec2(140,0)))
					TextureFormatCreation(static_cast<NetImgui::eTexFormat>(i));
				else if (mCustomTextureView[i] && ImGui::Button("Texture Destroy", ImVec2(140,0)))
					TextureFormatDestruction(static_cast<NetImgui::eTexFormat>(i));
				ImGui::SameLine();
				ImGui::Text("(%s)", gTextureFormatName[i]);	

				ImGui::SameLine(300.f);
				void* pTexture = mCustomTextureView[i] ? mCustomTextureView[i] : mDefaultEmptyTexture;
				ImTextureID TextureID;
				TextureID._TexUserID = reinterpret_cast<ImTextureUserID>(pTexture);
				ImGui::Image(TextureID, ImVec2(128, 64), ImVec2(0, 0), ImVec2(1, 1), tint_col, border_col);
				ImGui::PopID();
			}
			
			if( NetImgui::IsDrawingRemote() )
			{
				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Remote Server only Textures");

			//=====================================================================================
			// Note: This is a 2nd example of user adding their own texture format.
			//		 This one send the current time, and a sin wave is image generated.
			//		 It needs to be handled by adding the code in NetImguiServer 
			//		 inside the 'ProcessTexture_Custom' function.
			//		 If user change the Server function, then this sample code won't 
			//		 be compatible anymore. 
			#if TEXTURE_CUSTOM_SAMPLE
				customTextureData2 customTextureData;
				customTextureData.m_TimeUS = static_cast<uint64_t>(ImGui::GetTime() * 1000.f * 1000.f);
				NetImgui::SendDataTexture(ImTextureUserID(0x02020202), &customTextureData, 128, 64, NetImgui::eTexFormat::kTexFmtCustom, sizeof(customTextureData));

				ImGui::NewLine();
				ImGui::TextUnformatted("Custom Texture");
				ImGui::Image(ImTextureUserID(0x02020202), ImVec2(128, 64), ImVec2(0, 0), ImVec2(1, 1), tint_col, border_col);
				ImGui::SameLine();
				ImGui::TextWrapped("Server regenerate this texture every frame using a custom texture update and a time parameter. This behavior is custom implemented by library user on both the Client and Server codebase.");
			#endif
			//=====================================================================================

				ImGui::NewLine();
				ImGui::TextUnformatted("Invalid Texture");
				ImGui::Image(ImTextureID(0x01010101), ImVec2(128, 64), ImVec2(0, 0), ImVec2(1, 1), tint_col, border_col);
				ImGui::SameLine();
				ImGui::TextWrapped("Invalid textures handled by netImgui server by using a default invisible texture.");
		
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

