//=================================================================================================
// SAMPLE FONT DPI
//-------------------------------------------------------------------------------------------------
// Example of handling server monitor DPI in ImGui Rendering.
// We assign a callback to NetImgui to be notified on DPI change and regenerate the font
// accordingly. Without this, NetImgui Server relies on scaling the texture font 
// (using FontGlobalScale), creating blurier results.
//=================================================================================================

#include <NetImgui_Api.h>
#include "../Common/Sample.h"
#include "../../ServerApp/Source/Fonts/Roboto_Medium.cpp"
#include <math.h>

//=================================================================================================
// SAMPLE CLASS
//=================================================================================================
class SampleFontDPI : public SampleClient_Base
{
public:
						SampleFontDPI();
	virtual ImDrawData* Draw() override;
	virtual bool		UpdateFont(float fontScaleDPI, bool isLocal) override;

protected:
	void				Draw_Window(const char* WindowTitle, float FontDpiScaling);
};

//=================================================================================================
// GET SAMPLE
// Each project must return a valid sample object
//=================================================================================================
SampleClient_Base& GetSample()
{
	static SampleFontDPI sample;
	return sample;
}

//=================================================================================================
// Receive request to generate a Font to a specific monitor DPI.
// Useful when Server Application is running on high DPI display that would generate tiny text.
// When this is not supported, font is only scaled up on draw, creating blurrier text.
//=================================================================================================
void FontCreationCallback(float PreviousDPIScale, float NewDPIScale)
{
	IM_UNUSED(PreviousDPIScale);
// Note: If your codebase doesn't have access to recreating the Font Texture, you can instead 
// assign a new Font Atlas while connected to Server and re-assign the original FontAtlas 
// on disconnect (callback is called on connect/disconnect/DPI change).
//
// This is not needed in our samples and left as a demonstration
#if 1
	static ImFontAtlas* sOriginalFontAtlas(ImGui::GetIO().Fonts);
	if ( NetImgui::IsConnected() )
	{		
		if (ImGui::GetIO().Fonts == sOriginalFontAtlas) {
			ImGui::GetIO().Fonts = IM_NEW(ImFontAtlas)();
		}
	}
	// Restore the local display font atlas (see function's note)
	else if( ImGui::GetIO().Fonts != sOriginalFontAtlas )
	{
		if(ImGui::GetIO().Fonts != sOriginalFontAtlas ){
			IM_DELETE(ImGui::GetIO().Fonts);
			ImGui::GetIO().Fonts = sOriginalFontAtlas;
			ImGui::GetIO().Fonts->Clear(); // Make sure it is regenerated, in case there was a DPI change
		}
	}
#endif
	// Update the font data
	GetSample().UpdateFont(NewDPIScale, false);
}

//=================================================================================================
// CONSTRUCTOR
//=================================================================================================
SampleFontDPI::SampleFontDPI()
: SampleClient_Base("SampleFontDPI") 
{
	// For demonstration purposes, we replaced the default NetImgui Font DPI callback
	// used by our samples, to a new one, to demonstrate its usage
	mCallback_FontGenerate	= FontCreationCallback;
}

//=================================================================================================
// UPDATE FONT
//=================================================================================================
bool SampleFontDPI::UpdateFont(float fontScaleDPI, bool isLocal)
{
	// Ignore local DPI when drawing remotely
	// (font pixel should be dictated by remote server when sharing 1 context and connected)
	if( NetImgui::IsConnected() && isLocal ){
		return false;
	}

	// Iterate each font to detect when one requires a new pixel size because of the new DPI scaling
	// Note: Skip index 0 because this sample doesn't apply any scaling on it, to show difference
	ImFontAtlas* FontAtlas						= ImGui::GetIO().Fonts;
	bool bGenerate								= mGeneratedFontScaleDPI == 0.f || !FontAtlas->IsBuilt();
	const ImVector<ImFontConfig>& configData	= FontAtlas->ConfigData;
	for (int i(1); !bGenerate && i < configData.size(); ++i){
		float nativeSize	= roundf(configData[i].SizePixels / mGeneratedFontScaleDPI);
		bGenerate			= static_cast<int>(roundf(nativeSize * fontScaleDPI)) != static_cast<int>(configData[i].SizePixels);
	}

	// Recreate the Font Atlas
	// For the needs of this demo, we create 2x versions of the same font table, 
	// with 1 version using DPI scaling and 1 version without, to see the visual difference.
	if( bGenerate ){
		constexpr float sDefaultFontSize(16.f);
		constexpr float sRobotoFontSize(18.f);
		mGeneratedFontScaleDPI = fontScaleDPI;

		uint8_t* pPixelData(nullptr); int width(0), height(0);
		ImFontConfig FontConfig = {};
		FontAtlas->Clear();
		
		// 1st Font is Default Font using the DPI awareness
		FontConfig.SizePixels = roundf(sDefaultFontSize * fontScaleDPI);
		FontAtlas->AddFontDefault(&FontConfig);
		// 2nd Font is Roboto Font using the DPI awareness
		StringCopy(FontConfig.Name, "Roboto Medium");
		ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(Roboto_Medium_compressed_data, Roboto_Medium_compressed_size, sRobotoFontSize * fontScaleDPI, &FontConfig);

		// 3nd Font is Default Font without using the DPI awareness
		FontConfig.SizePixels	= sDefaultFontSize;
		FontAtlas->AddFontDefault(&FontConfig);
		// 4th Font is Roboto Font without using the DPI awareness
		StringCopy(FontConfig.Name, "Roboto Medium");
		ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(Roboto_Medium_compressed_data, Roboto_Medium_compressed_size, sRobotoFontSize, &FontConfig);
		
		// Regenerate the Font Texture (only if used by local context)
		FontAtlas->Build();
		FontAtlas->GetTexDataAsAlpha8(&pPixelData, &width, &height);
		NetImgui::SendDataTexture(FontAtlas->TexID, pPixelData, static_cast<uint16_t>(width), static_cast<uint16_t>(height), NetImgui::eTexFormat::kTexFmtA8);
		extern void ExtraSampleBackend_UpdateFontTexture();
		if( ImGui::GetCurrentContext() == mpContextLocal ){
			ExtraSampleBackend_UpdateFontTexture();
		}
		return true;
	}
	return false;
}

//=================================================================================================
// RemoteDraw UI Content
//=================================================================================================
void SampleFontDPI::Draw_Window(const char* WindowTitle, float FontDpiScaling)
{
	ImGui::SetNextWindowSize(ImVec2(640, 640), ImGuiCond_Appearing);
	if (ImGui::Begin(WindowTitle, nullptr))
	{
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Demonstration of high DPI monitor handling.");
		ImGui::TextWrapped("Shows the text quality difference when NetImGui is assigned a 'FontCreation Callback' and the server application is running on high DPI monitor.");
		ImGui::NewLine();
		ImGui::TextWrapped("Font with DPI awareness active are sharper since they are generated at a higher pixel count instead of scaling up the texture when drawing.");

		float alignSize = ImGui::CalcTextSize("Font generated size-").x;				
		ImGui::Text("Font draw scale "); ImGui::SameLine(alignSize); ImGui::Text(": %.02f", ImGui::GetIO().FontGlobalScale);
		ImGui::TextUnformatted("Font DPI Scaling"); ImGui::SameLine(alignSize); ImGui::Text(": %.02f", FontDpiScaling);
		ImGui::TextUnformatted("Font Pixel Size "); ImGui::SameLine(alignSize); ImGui::Text(": %.02f", ImGui::GetFont()->ConfigData->SizePixels);
		ImGui::SameLine(); ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.75f, 1.f), " (Regular Size * DPI Scaling)");

		ImGui::NewLine();
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Filler content");
		ImGui::TextUnformatted("abcdefghijklmnopqrstuvwxyz01234567890-+=.;<>");
		ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
	}
	ImGui::End();
}

//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
ImDrawData* SampleFontDPI::Draw()
{
	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame.
	// Note:	With ImGui 1.81+ NetImgui can automatically intercept Imgui::NewFrame/Render. This
	//			sample does this. For older Imgui releases, please look at 'Client_Draw_ModeAlways'
	//			in 'SampleNewFrame' on how to tell NetImgui directly about NewFrame/EndFrame.
	//			Other samples also avoid the auto intercept to allow drawing only when needed.
	//---------------------------------------------------------------------------------------------
	ImGui::NewFrame();

	//-----------------------------------------------------------------------------------------
	// (2) Draw ImGui Content
	//-----------------------------------------------------------------------------------------
	SampleClient_Base::Draw_Connect(); //Note: Connection to remote server done in there

	ImGui::SetNextWindowPos(ImVec2(32,48), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(400,400), ImGuiCond_Once);
	ImGui::GetFont()->Scale = 1;

	//-----------------------------------------------------------------------------------------
	// (2) Draw ImGui Content
	//-----------------------------------------------------------------------------------------
	if (ImGui::Begin("Sample FontDPI", nullptr))
	{
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Demonstration of high DPI monitor handling.");
		ImGui::TextWrapped("Shows the text quality difference when NetImGui is assigned a 'FontCreation Callback' and the server application is running on high DPI monitor.");
	}
	ImGui::End();

	ImFontAtlas* fontAtlas	= ImGui::GetIO().Fonts;
	float dpiScaling		= fontAtlas->ConfigData[0].SizePixels / fontAtlas->ConfigData[2].SizePixels;
	float savedScale		= ImGui::GetIO().FontGlobalScale;
	
	// First half of our Fonts are generated with DPI Scaling applied
	ImGui::PushFont(fontAtlas->Fonts[0]);
	Draw_Window("Sample Font 1 [DPI Scaling On]", dpiScaling);
	ImGui::PopFont();

	ImGui::PushFont(fontAtlas->Fonts[1]);
	Draw_Window("Sample Font 2 [DPI Scaling On]", dpiScaling);
	ImGui::PopFont();

	// 2nd half of our font generated without DPI scaling support (relies on texture scaling instead)
	ImGui::GetIO().FontGlobalScale = dpiScaling;
	ImGui::PushFont(fontAtlas->Fonts[2]);
	Draw_Window("Sample Font 1 [DPI Scaling Off]", 1.f);
	ImGui::PopFont();
	
	ImGui::PushFont(fontAtlas->Fonts[3]);
	Draw_Window("Sample Font 2 [DPI Scaling Off]", 1.f);
	ImGui::PopFont();
	ImGui::GetIO().FontGlobalScale = savedScale;

	//---------------------------------------------------------------------------------------------
	// (3) Finish the frame
	// Note:	Same note as in (1)
	//---------------------------------------------------------------------------------------------
	ImGui::Render();

	//---------------------------------------------------------------------------------------------
	// (4) Return content to draw by local renderer. Stop drawing locally when remote connected
	//---------------------------------------------------------------------------------------------
	return !NetImgui::IsConnected() ? ImGui::GetDrawData() : nullptr;
}
