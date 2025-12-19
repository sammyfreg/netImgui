//=================================================================================================
// SAMPLE TEXTURES
//-------------------------------------------------------------------------------------------------
// Example of using various textures in the ImGui context, after making netImgui aware of them.
//=================================================================================================

#include <NetImgui_Api.h>
#include <array>
#include "../Common/Sample.h"

#include "imgui_internal.h" //SF TODO hide behind define?

// Enable handling of a Custom Texture Format samples. 
// Only meant as an example, users are free to replace it with their own handling of
// custom texture formats. Look for this define for implementation details.
#define TEXTURE_CUSTOM_SAMPLE 0 //SF TODO re-implement this

// Methods declared in main.cpp, extern declare to avoid having to include 'd3d11.h' here
extern void TextureCreate(const uint8_t* pPixelData, uint32_t width, uint32_t height, void*& pTextureViewOut);
extern void TextureDestroy(void*& pTextureView);

inline ImTextureID TextureCastFromPtr(const void* TexturePtr)
{
	union CastHelper
	{
		uint64_t	TexUint;
		const void*	TexPtr;
		ImTextureID TexID;
	} castHelper;
	
	castHelper.TexUint 	= 0;
	castHelper.TexPtr	= TexturePtr;
	return castHelper.TexID;
}

//=================================================================================================
// SAMPLE CLASS
//=================================================================================================
class SampleTextures : public Sample::Base
{
public:
						SampleTextures() : Base("SampleTextures") {}
	virtual bool		Startup() override;
	virtual void		Shutdown() override;
	virtual void		Draw() override;
protected:
	void				TextureFormatCreation(NetImgui::eTexFormat eTexFmt);
	void				TextureFormatDestruction(NetImgui::eTexFormat eTexFmt);
	void*				mDefaultEmptyTexture = nullptr;
	void*				mCustomTextureView[static_cast<int>(NetImgui::eTexFormat::kTexFmt_Count)];

	static constexpr uint32_t	kTexSize = 64;
	uint8_t				mTexDataNormal_R8[kTexSize*kTexSize];
	uint8_t				mTexDataNormal_RGBA32[4*kTexSize*kTexSize];
	uint8_t				mTexDataDoubleSendA_RGBA32[4*kTexSize*kTexSize];
	uint8_t				mTexDataDoubleSendB_RGBA32[4*kTexSize*kTexSize];
	ImTextureRef		mNativeImTextureRef;
	ImTextureData		mNativeImTextureData;
	uint8_t				mNativeImTextureUpdateCount = 0;
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
		NetImgui::SendDataTexture(TextureCastFromPtr(mCustomTextureView[static_cast<int>(eTexFmt)]), &customTextrueData, Width, Height, eTexFmt, sizeof(customTextureData1));	// For remote display
	}
	else
#endif
//=================================================================================================
// 
	//----------------------------------------------------------------------------
	// Normal Texture Handling (of normal supported formats)
	//----------------------------------------------------------------------------
	{
		uint8_t* pixelData(nullptr);
		switch (eTexFmt)
		{
		case NetImgui::eTexFormat::kTexFmtA8: 	pixelData = mTexDataNormal_R8; break;
		case NetImgui::eTexFormat::kTexFmtRGBA8:pixelData = mTexDataNormal_RGBA32; break;
		case NetImgui::eTexFormat::kTexFmtCustom: break;
		case NetImgui::eTexFormat::kTexFmt_Invalid: assert(0); break;
		}

		if( pixelData )
		{
			TextureCreate(pixelData, kTexSize, kTexSize, mCustomTextureView[static_cast<int>(eTexFmt)]);											// For local display
			//SF remove this when using backend texture support ?
			NetImgui::SendDataTexture(TextureCastFromPtr(mCustomTextureView[static_cast<int>(eTexFmt)]), pixelData, kTexSize, kTexSize, eTexFmt);	// For remote display
		}
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
		NetImgui::SendDataTexture(TextureCastFromPtr(mCustomTextureView[static_cast<int>(eTexFmt)]), nullptr, 0, 0, NetImgui::eTexFormat::kTexFmt_Invalid);
		
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
	NetImgui::SendDataTexture(TextureCastFromPtr(mDefaultEmptyTexture), pixelData.data(), 8, 8, NetImgui::eTexFormat::kTexFmtRGBA8);

	// Init the texture data once, to re-use it during texture creation
	for (uint8_t y(0); y < kTexSize; ++y)
	{
		for (uint8_t x(0); x < kTexSize; ++x)
		{
			mTexDataNormal_R8[(y * kTexSize + x)] = 0xFF * x / kTexSize;

			mTexDataNormal_RGBA32[(y * kTexSize + x) * 4 + 0] = 0xFF * x / kTexSize;
			mTexDataNormal_RGBA32[(y * kTexSize + x) * 4 + 1] = 0xFF * y / kTexSize;
			mTexDataNormal_RGBA32[(y * kTexSize + x) * 4 + 2] = 0xFF;
			mTexDataNormal_RGBA32[(y * kTexSize + x) * 4 + 3] = 0xFF;

			mTexDataDoubleSendA_RGBA32[(y * kTexSize + x) * 4 + 0] = 0xFF * x / kTexSize;
			mTexDataDoubleSendA_RGBA32[(y * kTexSize + x) * 4 + 1] = 0x00;
			mTexDataDoubleSendA_RGBA32[(y * kTexSize + x) * 4 + 2] = 0x00;
			mTexDataDoubleSendA_RGBA32[(y * kTexSize + x) * 4 + 3] = 0xFF;

			mTexDataDoubleSendB_RGBA32[(y * kTexSize + x) * 4 + 0] = 0x00;
			mTexDataDoubleSendB_RGBA32[(y * kTexSize + x) * 4 + 1] = 0xFF * y / kTexSize;
			mTexDataDoubleSendB_RGBA32[(y * kTexSize + x) * 4 + 2] = 0x00;
			mTexDataDoubleSendB_RGBA32[(y * kTexSize + x) * 4 + 3] = 0xFF;
		}
	}

	// Init the new native ImGui texture support (for backend that support it)
	mNativeImTextureRef._TexData = &mNativeImTextureData;
	mNativeImTextureData.Create(ImTextureFormat::ImTextureFormat_RGBA32, kTexSize, kTexSize);
	mNativeImTextureData.Status = ImTextureStatus_WantCreate;
	mNativeImTextureData.RefCount++;
	memcpy(mNativeImTextureData.Pixels, mTexDataNormal_RGBA32, mNativeImTextureData.GetSizeInBytes());

	ImGui::RegisterUserTexture(&mNativeImTextureData);

	return true;
}

//=================================================================================================
//
//=================================================================================================
void SampleTextures::Shutdown()
{
	mNativeImTextureData.RefCount--;
	ImGui::UnregisterUserTexture(&mNativeImTextureData);
	NetImgui::SendDataTexture(TextureCastFromPtr(mDefaultEmptyTexture), nullptr, 0, 0, NetImgui::eTexFormat::kTexFmt_Invalid);
	TextureDestroy(mDefaultEmptyTexture);
	Base::Shutdown();
}

//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
void SampleTextures::Draw()
{
	const char*	gTextureFormatName[static_cast<int>(NetImgui::eTexFormat::kTexFmt_Count)] = { "RGBA8", "R8", "Custom"};
	//bool bCanDisplayLocally(false);

	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame
	//---------------------------------------------------------------------------------------------
	if (NetImgui::NewFrame(true))
	{		
		//-----------------------------------------------------------------------------------------
		// (2) Draw ImGui Content
		//-----------------------------------------------------------------------------------------
		Base::Draw_Connect(); //Note: Connection to remote server done in there

		constexpr int kBtnWidth = 200.f;
		ImGui::SetNextWindowPos(ImVec2(32, 48), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_Once);
		if (ImGui::Begin("Sample Textures", nullptr))
		{
			ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Demonstration of textures usage with netImgui.");	
			ImGui::TextWrapped("Note: Textures properly displayed on netImgui server only. Original 'Dear ImGui' sample code was left intact for easier maintenance and is limited to the RGBA format. This is not a NetImgui limitation.");
			ImGui::NewLine();
			for (int i = 0; i < static_cast<int>(NetImgui::eTexFormat::kTexFmt_Count); ++i)
			{
				// Only display this on remote connection (associated texture note created locally)
				ImGui::PushID(i);
				if (!mCustomTextureView[i] && ImGui::Button("Texture Create", ImVec2(kBtnWidth,0)))
					TextureFormatCreation(static_cast<NetImgui::eTexFormat>(i));
				else if (mCustomTextureView[i] && ImGui::Button("Texture Destroy", ImVec2(kBtnWidth,0)))
					TextureFormatDestruction(static_cast<NetImgui::eTexFormat>(i));
				ImGui::SameLine();
				ImGui::Text("(%s)", gTextureFormatName[i]);	

				ImGui::SameLine(300.f);
				void* pTexture = mCustomTextureView[i] ? mCustomTextureView[i] : mDefaultEmptyTexture;
				ImTextureRef TextureRef;
				TextureRef._TexID = TextureCastFromPtr(pTexture);
				ImGui::Image(TextureRef, ImVec2(128, 64), ImVec2(0, 0), ImVec2(1, 1));
				ImGui::PopID();
			}

			// Update the texture lower/right corner colors
			// This test the new update feature of native ImTexture with backend support
			if( ImGui::Button("ImTexture Update", ImVec2(kBtnWidth,0)))
			{
				// ABGR format
				constexpr uint32_t kColorCount	= 4;
				constexpr uint32_t kColors[kColorCount][2]={{0x00000000, 0xFFFFFFFF}, {0xFF0000FF, 0x00000000}, {0x00000000, 0xFF00FF00}, {0xFFFF0000, 0x00000000}};
				uint32_t NewColorA 	= kColors[mNativeImTextureUpdateCount%kColorCount][0];
				uint32_t NewColorB 	= kColors[mNativeImTextureUpdateCount%kColorCount][1];
				ImTextureRect req 	= {(unsigned short)kTexSize/2, (unsigned short)kTexSize/2, (unsigned short)kTexSize/2, (unsigned short)kTexSize/2 };
				ImTextureData* tex 	= &mNativeImTextureData;
				mNativeImTextureUpdateCount++;

				for (int y = 0; y < req.h; ++y){
					uint32_t* TexColorDataRow = reinterpret_cast<uint32_t*>(mNativeImTextureData.GetPixelsAt(req.x, req.y+y));
					for (int x = 0; x < req.w; ++x){
						TexColorDataRow[x] = (((x / 8) & 0x01) ^ ((y / 8) & 0x01)) ? NewColorB : NewColorA;
					}
				}
			
				int new_x1 = ImMax(tex->UpdateRect.w == 0 ? 0 : tex->UpdateRect.x + tex->UpdateRect.w, req.x + req.w);
				int new_y1 = ImMax(tex->UpdateRect.h == 0 ? 0 : tex->UpdateRect.y + tex->UpdateRect.h, req.y + req.h);
				tex->UpdateRect.x = ImMin(tex->UpdateRect.x, req.x);
				tex->UpdateRect.y = ImMin(tex->UpdateRect.y, req.y);
				tex->UpdateRect.w = (unsigned short)(new_x1 - tex->UpdateRect.x);
				tex->UpdateRect.h = (unsigned short)(new_y1 - tex->UpdateRect.y);
				tex->UsedRect.x = ImMin(tex->UsedRect.x, req.x);
				tex->UsedRect.y = ImMin(tex->UsedRect.y, req.y);
				tex->UsedRect.w = (unsigned short)(ImMax(tex->UsedRect.x + tex->UsedRect.w, req.x + req.w) - tex->UsedRect.x);
				tex->UsedRect.h = (unsigned short)(ImMax(tex->UsedRect.y + tex->UsedRect.h, req.y + req.h) - tex->UsedRect.y);

				// No need to queue if status is _WantCreate
				if (tex->Status != ImTextureStatus_WantCreate)
				{
					tex->Status = ImTextureStatus_WantUpdates;
					tex->Updates.push_back(req);
				}
			}
			ImGui::SameLine();
			ImGui::Image(mNativeImTextureRef, ImVec2(128, 64), ImVec2(0, 0), ImVec2(1, 1));

			if ( ImGui::Button("Texture Overwrite", ImVec2(kBtnWidth,0)))
			{
				TextureFormatCreation(NetImgui::eTexFormat::kTexFmtRGBA8);
				//NetImgui::SendDataTexture(TextureCastFromPtr(mCustomTextureView[static_cast<int>(NetImgui::eTexFormat::kTexFmtRGBA8)]), &mTexDataNormal_RGBA32, kTexSize, kTexSize, NetImgui::eTexFormat::kTexFmtRGBA8);
				TextureFormatCreation(NetImgui::eTexFormat::kTexFmtRGBA8);
			}
			ImGui::SetItemTooltip("Test creating same texture ID 2x in one frame, making sure the Client code and Server handles it properly");

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
				NetImgui::SendDataTexture(ImTextureID(0x02020202), &customTextureData, 128, 64, NetImgui::eTexFormat::kTexFmtCustom, sizeof(customTextureData));

				ImGui::NewLine();
				ImGui::TextUnformatted("Custom Texture");
				ImGui::ImageWithBg(ImTextureID(0x02020202), ImVec2(128, 64), ImVec2(0, 0), ImVec2(1, 1), tint_col, border_col);
				ImGui::SameLine();
				ImGui::TextWrapped("Server regenerate this texture every frame using a custom texture update and a time parameter. This behavior is custom implemented by library user on both the Client and Server codebase.");
			#endif
			//=====================================================================================

				ImGui::NewLine();
				ImGui::TextUnformatted("Invalid Texture");
				ImGui::Image(ImTextureID(0x01010101), ImVec2(128, 64), ImVec2(0, 0), ImVec2(1, 1));
				ImGui::SameLine();
				ImGui::TextWrapped("Invalid textures handled by netImgui server by using a default invisible texture.");
		
				// Invalid textures not handled by the base Dear ImGui Sample code, but netImgui server can.
				// Making sure we are not trying to draw an invalid texture locally, after a disconnect from remote server
				//bCanDisplayLocally = false;
			}
			else
			{
				//bCanDisplayLocally = true;
			}
		}
		ImGui::End();

		//-----------------------------------------------------------------------------------------
		// (3) Finish the frame, preparing the drawing data and...
		// (3a) Send the data to the netImGui server when connected
		//-----------------------------------------------------------------------------------------
		NetImgui::EndFrame();
	}
}

