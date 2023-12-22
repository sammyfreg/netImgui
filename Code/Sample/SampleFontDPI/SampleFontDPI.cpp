//=================================================================================================
// SAMPLE FONT DPI
//-------------------------------------------------------------------------------------------------
// Example of handling server monitor DPI in ImGui Rendering.
// We assign a callback to NetImgui to be notified on DPI change and regenerate the font
// accordingly. Without this, NetImgui Server relies on scaling the texture font 
// (using FontGlobalScale), creating blurier results.
//=================================================================================================

#include <NetImgui_Api.h>
#include <Private/NetImgui_Shared.h> // For StringCopy
#include "../Common/Sample.h"
#include "../../ServerApp/Source/Fonts/Roboto_Medium.cpp"
#include <math.h>

namespace SampleClient
{

//=================================================================================================
// Receive request to generate a Font to a specific monitor DPI.
// Useful when Server Application is running on high DPI display that would generate tiny text.
// When this is not supported, font is only scaled up on draw, creating blurrier text.
//
// Note: The sample doesn't have access to code to regenerate the Font Texture
// (to avoid modifying the ImGui sample code too much, and using platform specific code)
// Instead we do not handle font DPI locally. and create a new Font Atlas for the remote connection.
// On disconnect, we re-assign the original FontAtlas. This is only a limitation of the sample code
// and code base with font texture access, can keep using the same Font Atlas and generate the
// font texture on demand.
//=================================================================================================
void FontCreationCallback(float PreviousDPIScale, float NewDPIScale)
{
	static ImFontAtlas* sOriginalFontAtlas(nullptr);
	static float sDefaultFontSize(0.f);
	constexpr float sRobotoFontSize(18.f);
	if( !sOriginalFontAtlas ){
		sOriginalFontAtlas	= ImGui::GetIO().Fonts;
		sDefaultFontSize	= ImGui::GetIO().Fonts->ConfigData[0].SizePixels;
	}

	if ( !NetImgui::IsConnected() )
	{
		// Restore the local display font atlas (see function's note)
		if(ImGui::GetIO().Fonts != sOriginalFontAtlas ){
			IM_DELETE(ImGui::GetIO().Fonts);
			ImGui::GetIO().Fonts = sOriginalFontAtlas;
		}
	}
	else
	{
		// Replace the original font atlas with a remote only atlas (see function's note)
		bool bGenerate(false);
		if (ImGui::GetIO().Fonts == sOriginalFontAtlas) {
			ImGui::GetIO().Fonts	= IM_NEW(ImFontAtlas)();
			bGenerate				= true;
		}

		// Iterate each font to detect when one requires a new pixel size because of the new DPI scaling
		// Note: Skip index 0 because this sample doesn't apply any scaling on it, to show difference
		ImFontAtlas* FontAtlas						= ImGui::GetIO().Fonts;
		const ImVector<ImFontConfig>& configData	= FontAtlas->ConfigData;
		for (int i(1); i < configData.size(); ++i){
			float nativeSize	= roundf(configData[i].SizePixels / PreviousDPIScale);
			bGenerate			= static_cast<int>(roundf(nativeSize * NewDPIScale)) != static_cast<int>(configData[i].SizePixels);
		}

		// Generate the Font
		if (bGenerate)
		{
			uint8_t* pPixelData(nullptr); int width(0), height(0);
			ImFontConfig FontConfig = {};
			FontAtlas->Clear();
			// 1st Font is Default Font using the DPI awareness
			FontConfig.SizePixels = roundf(sDefaultFontSize * NewDPIScale);
			FontAtlas->AddFontDefault(&FontConfig);
			// 2nd Font is Roboto Font using the DPI awareness
			NetImgui::Internal::StringCopy(FontConfig.Name, "Roboto Medium");
			ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(Roboto_Medium_compressed_data, Roboto_Medium_compressed_size, sRobotoFontSize * NewDPIScale, &FontConfig);

			// 3nd Font is Default Font without using the DPI awareness
			FontConfig.SizePixels	= sDefaultFontSize;
			FontAtlas->AddFontDefault(&FontConfig);
			// 4th Font is Roboto Font without using the DPI awareness
			NetImgui::Internal::StringCopy(FontConfig.Name, "Roboto Medium");
			ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(Roboto_Medium_compressed_data, Roboto_Medium_compressed_size, sRobotoFontSize, &FontConfig);

			FontAtlas->SetTexID(0);
			FontAtlas->Build();
			FontAtlas->GetTexDataAsAlpha8(&pPixelData, &width, &height);
			NetImgui::SendDataTexture(FontAtlas->TexID, pPixelData, static_cast<uint16_t>(width), static_cast<uint16_t>(height), NetImgui::eTexFormat::kTexFmtA8);
		}
	}
}

//=================================================================================================
//
//=================================================================================================
bool Client_Startup()
{
	if( !NetImgui::Startup() )
		return false;

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
// LocalDraw UI Content
//=================================================================================================
void Client_Draw_Local()
{
	if (ImGui::Begin("Sample FontDPI", nullptr))
	{
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Demonstration of high DPI monitor handling.");
		ImGui::TextWrapped("Shows the text quality difference when NetImGui is assigned a 'FontCreation Callback' and the server application is running on high DPI monitor.");
		
		ImGui::NewLine();		
		ImGui::TextWrapped("Note: This font resizing only work on the remote server connection. Monitor DPI has not been implemented on this Sample's local display.");
	}
	ImGui::End();
}

//=================================================================================================
// RemoteDraw UI Content
//=================================================================================================
void Client_Draw_Window(const char* WindowTitle, float FontDpiScaling)
{
	ImGui::SetNextWindowSize(ImVec2(640, 640), ImGuiCond_Appearing);
	if (ImGui::Begin(WindowTitle, nullptr))
	{
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Demonstration of high DPI monitor handling.");
		ImGui::TextWrapped("Shows the text quality difference when NetImGui is assigned a 'FontCreation Callback' and the server application is running on high DPI monitor.");
		ImGui::NewLine();
		ImGui::TextWrapped("Font with DPI awareness active are sharper since they are generated at a higher pixel count instead of scaling up the texture when drawing.");

		float alignSize = ImGui::CalcTextSize("Font generated size-").x;
		ImGui::TextUnformatted("Font DPI Scaling"); ImGui::SameLine(alignSize); ImGui::Text(": %.02f", FontDpiScaling);
		ImGui::TextUnformatted("Font pixel size "); ImGui::SameLine(alignSize); ImGui::Text(": %.02f", ImGui::GetFont()->ConfigData->SizePixels);
		ImGui::Text("Font draw scale "); ImGui::SameLine(alignSize); ImGui::Text(": %.02f", ImGui::GetIO().FontGlobalScale);

		ImGui::NewLine();
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Filler content");
		ImGui::TextUnformatted("abcdefghijklmnopqrstuvwxyz01234567890-+=.;<>");
		ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
	}
	ImGui::End();
}

//=================================================================================================
// RemoteDraw UI Content
//=================================================================================================
void Client_Draw_Remote()
{	
	ImFontAtlas* fontAtlas	= ImGui::GetIO().Fonts;
	float dpiScaling		= fontAtlas->ConfigData[0].SizePixels / fontAtlas->ConfigData[2].SizePixels;
	float savedScale		= ImGui::GetIO().FontGlobalScale;
	
	// First half of our Fonts are generated with DPI Scaling applied
	ImGui::PushFont(fontAtlas->Fonts[0]);
	Client_Draw_Window("Sample Font 1 [DPI Scaling On]", dpiScaling);
	ImGui::PopFont();

	ImGui::PushFont(fontAtlas->Fonts[1]);
	Client_Draw_Window("Sample Font 2 [DPI Scaling On]", dpiScaling);
	ImGui::PopFont();

	// 2nd half of our font generated without DPI scaling support (relies on texture scaling instead)
	ImGui::GetIO().FontGlobalScale = dpiScaling;
	ImGui::PushFont(fontAtlas->Fonts[2]);
	Client_Draw_Window("Sample Font 1 [DPI Scaling Off]", 1.f);
	ImGui::PopFont();
	
	ImGui::PushFont(fontAtlas->Fonts[3]);
	Client_Draw_Window("Sample Font 2 [DPI Scaling Off]", dpiScaling);
	ImGui::PopFont();
	ImGui::GetIO().FontGlobalScale = savedScale;
}


//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
ImDrawData* Client_Draw()
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
	ClientUtil_ImGuiContent_Common("SampleFontDPI", nullptr, FontCreationCallback); //Note: Connection to remote server done in there

	ImGui::SetNextWindowPos(ImVec2(32,48), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(400,400), ImGuiCond_Once);
	ImGui::GetFont()->Scale = 1;

	//-----------------------------------------------------------------------------------------
	// (2) Draw ImGui Content
	//-----------------------------------------------------------------------------------------
	if( NetImgui::IsDrawingRemote() && ImGui::GetIO().Fonts->Fonts.size() > 1 ){
		Client_Draw_Remote();
	}
	else{
		Client_Draw_Local();
	}
	
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

} // namespace SampleClient
