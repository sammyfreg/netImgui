//=================================================================================================
// SAMPLE BACKGROUND
//-------------------------------------------------------------------------------------------------
// Example of using the NetImgui 'Background' setting
//=================================================================================================

#include <NetImgui_Api.h>
#include <array>
#include <cmath>
#include "../Common/Sample.h"
#include "../Common/TextureResource.h"

//=================================================================================================
// SAMPLE CLASS
//=================================================================================================
class SampleBackground : public Sample::Base
{
public:
						SampleBackground() : Base("SampleBackground"){}
	virtual bool		Startup() override;
	virtual void		Shutdown() override;
	virtual void		Draw() override;
protected:
	TexResImgui			mTexture;
};

//=================================================================================================
// GET SAMPLE
// Each project must return a valid sample object
//=================================================================================================
Sample::Base& GetSample()
{
	static SampleBackground sample;
	return sample;
};

//=================================================================================================
// STARTUP
//=================================================================================================
bool SampleBackground::Startup()
{
	if (!Base::Startup())
		return false;

	constexpr uint16_t kSize	= 256;
	constexpr float kFadeSize	= 16.f;
	mTexture.InitPixels(ImTextureFormat::ImTextureFormat_RGBA32, 256, 256);
	for (int y(0); y < mTexture.mImTexData.Height; ++y)
	{
		uint32_t* pPixRow 	= static_cast<uint32_t*>(mTexture.mImTexData.GetPixelsAt(0,y));
		float offsetY		= static_cast<float>(y) - static_cast<float>(kSize/2);
		for (int x(0); x < kSize; ++x)
		{
			float offsetX	= static_cast<float>(x) - static_cast<float>(kSize/2);
			float radius	= static_cast<float>(sqrt(offsetX*offsetX+offsetY*offsetY));
			float alpha		= 1.f - std::min(1.f, std::max(0.f, (radius - (static_cast<float>(kSize/2)-kFadeSize))) / kFadeSize);
			pPixRow[x]		= ImColor( static_cast<uint8_t>(x), static_cast<uint8_t>(y), (x+y) <= 255 ? 0 : 255-(x+y), static_cast<uint8_t>(255.f*alpha));
		}
	}
	mTexture.Create();
	return true;
}

//=================================================================================================
// SHUTDOWN
//=================================================================================================
void SampleBackground::Shutdown()
{
	NetImgui::Shutdown();
}

//=================================================================================================
// DRAW
//=================================================================================================
void SampleBackground::Draw()
{
	mTexture.Update();

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
		ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);
		if (ImGui::Begin("Sample Background", nullptr))
		{
			ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Demonstration of NetImgui's Background settings.");	
			static ImVec4 sBgColor(0.2f,0.2f,0.2f,1.f);
			static ImVec4 sTextureTint(1,1,1,0.5f);
			static bool sUseTextureOverride(false);
			bool bChanged(false);
			bChanged |= ImGui::ColorEdit4("Background", reinterpret_cast<float*>(&sBgColor), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_PickerHueWheel);
			bChanged |= ImGui::ColorEdit4("Logo Tint", reinterpret_cast<float*>(&sTextureTint), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_PickerHueWheel);			
			bChanged |= ImGui::Checkbox("Replace Background Texture", &sUseTextureOverride);
			ImGui::Image(mTexture.mImTexRef, ImVec2(64,64));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
			ImGui::TextWrapped("(Note: Custom background settings only applied on remote server)");
			ImGui::PopStyleColor();
			if( bChanged )
			{
				if( sUseTextureOverride ){
					NetImgui::SetBackground(sBgColor, sTextureTint, mTexture.mImTexRef);
				}
				else{ 
					NetImgui::SetBackground(sBgColor, sTextureTint);
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
}

