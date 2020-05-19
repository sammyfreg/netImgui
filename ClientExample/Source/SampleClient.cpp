#include <NetImgui_Api.h>
#include "SampleClient.h"

#include "Private/NetImgui_WarningDisableStd.h"
#include <chrono>
#include <thread>
#include "Private/NetImgui_WarningReenable.h"

#include "Private/NetImgui_WarningDisable.h"


// Methods declared in main.cpp, to avoid having to include 'd3d11.h' here
extern void TextureCreate(const uint8_t* pPixelData, uint32_t width, uint32_t height, void*& pTextureViewOut);
extern void TextureDestroy(void*& pTextureView);

namespace SampleClient
{

static int			gConnectPort		= NetImgui::kDefaultServerPort;
static char			gConnectIP[64]		= "127.0.0.1";
static ImDrawData*	gpLastRemoteDraw	= nullptr;
static bool			gEnableRemoteMirror	= false;
static void*		gCustomTextureView	= nullptr;

void				Imgui_DrawMainMenu();
void				Imgui_DrawContent(ImVec4& clear_col);
void				Imgui_DrawContentSecondary();
void				Client_DrawLocal(ImVec4& clearColorOut);
void				Client_DrawRemote(ImVec4& clearColorOut);
void				CustomCommunicationThread(void ComFunctPtr(void*), void* pClient);
void				CustomTextureCreate();
void				CustomTextureDestroy();

//=================================================================================================
//! @brief		Custom Connect thread example
//! @details	This function demonstrate own to provide your own function to start a new thread that
//!				will handle communication with remote netImgui application. 
//!				The default implementation also use std::thread
//=================================================================================================
void CustomCommunicationThread( void ComFunctPtr(void*), void* pClient )
{
	std::thread(ComFunctPtr, pClient).detach();
}

//=================================================================================================
//
//=================================================================================================
void CustomTextureCreate()
{
	constexpr uint32_t Width = 8;
	constexpr uint32_t Height = 8;
	uint8_t pixelData[Width * Height * 4];
	for (uint8_t y(0); y < Height; ++y)
	{
		for (uint8_t x(0); x < Width; ++x)
		{
			pixelData[(y * Width + x) * 4 + 0] = 0xFF * x / 8u;
			pixelData[(y * Width + x) * 4 + 1] = 0xFF * y / 8u;
			pixelData[(y * Width + x) * 4 + 2] = 0xFF;
			pixelData[(y * Width + x) * 4 + 3] = 0xFF;
		}
	}	
	TextureCreate(pixelData, Width, Height, gCustomTextureView);
	NetImgui::SendDataTexture(reinterpret_cast<uint64_t>(gCustomTextureView), pixelData, 8, 8, NetImgui::kTexFmtRGBA8);
}

//=================================================================================================
//
//=================================================================================================
void CustomTextureDestroy()
{
	
	if( gCustomTextureView )
	{
		NetImgui::SendDataTexture(reinterpret_cast<uint64_t>(gCustomTextureView), nullptr, 0, 0, NetImgui::kTexFmt_Invalid);
		TextureDestroy(gCustomTextureView);
	}
}

//=================================================================================================
//
//=================================================================================================
bool Client_Startup()
{
	if( !NetImgui::Startup() )
		return false;

    // Load Fonts
    // (see extra_fonts/README.txt for more details)
    //ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

    // Merge glyphs from multiple fonts into one (e.g. combine default font with another with Chinese glyphs, or add icons)
    //ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 };
    //ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/fontawesome-webfont.ttf", 18.0f, &icons_config, icons_ranges);

	// Save the default Imgui State and create a secondary one for use locally while debug menu displayed in netImguiApp		
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
//
//=================================================================================================
void Client_AddFontTexture(uint64_t texId, void* pData, uint16_t width, uint16_t height)
{
	NetImgui::SendDataTexture(texId, pData, width, height, NetImgui::kTexFmtRGBA8 );
}

//=================================================================================================
// 
//=================================================================================================
ImDrawData* Client_Draw(ImVec4& clearColorOut)
{
	Client_DrawLocal(clearColorOut);
	Client_DrawRemote(clearColorOut);
	return NetImgui::IsConnected() && gEnableRemoteMirror && gpLastRemoteDraw ? gpLastRemoteDraw : ImGui::GetDrawData();
}


//=================================================================================================
//
//=================================================================================================
void Client_DrawLocal(ImVec4& clearColorOut)
{
	if( (NetImgui::IsConnected() && gEnableRemoteMirror) == false )
	{
		ImGui::NewFrame();
		if( !NetImgui::IsConnected() )
		{		
			SampleClient::Imgui_DrawMainMenu();
			SampleClient::Imgui_DrawContent(clearColorOut);
		}
		else
		{
			SampleClient::Imgui_DrawMainMenu();
			SampleClient::Imgui_DrawContentSecondary();
		}
		ImGui::Render();
	}
}

//=================================================================================================
// Normal DebugUI rendering
// Can either be displayed locally or sent to remote netImguiApp server if connected
//=================================================================================================
void Client_DrawRemote(ImVec4& clearColorOut)
{		
	if( NetImgui::NewFrame() )
	{		
		SampleClient::Imgui_DrawMainMenu();
		SampleClient::Imgui_DrawContent(clearColorOut);
		gpLastRemoteDraw = const_cast<ImDrawData*>(NetImgui::EndFrame()); //!< Note: the example function 'ImGui_ImplDX11_RenderDrawData' provided by imgui does not take a const object
	}
}

void Imgui_DrawMainMenu()
{
	ImGui::BeginMainMenuBar();
	if( NetImgui::IsConnected() )
	{
		if( ImGui::Button("Disconnect") )
			NetImgui::Disconnect();

		ImGui::Checkbox("Mirror remote", &gEnableRemoteMirror);
		if( gEnableRemoteMirror )
			ImGui::TextUnformatted("(Can't control Imgui locally)");
	}
	else
	{		
		ImGui::Text("IP:");ImGui::SameLine();
		ImGui::PushItemWidth(100); ImGui::InputText("HostIP", gConnectIP, sizeof(gConnectIP)); ImGui::PopItemWidth(); ImGui::SameLine();
		ImGui::Text("Port:"); ImGui::SameLine();
		ImGui::PushItemWidth(100); ImGui::InputInt("##PORT", &gConnectPort); ImGui::PopItemWidth(); ImGui::SameLine();
		if( ImGui::Button("Connect") )
			NetImgui::Connect(CustomCommunicationThread, "SampleClientPC", gConnectIP, static_cast<uint32_t>(gConnectPort));
	}
	ImGui::EndMainMenuBar();
}

void Imgui_DrawContent(ImVec4& clear_col)
{
	// Straight from DearImgui sample
	static bool show_test_window	= true;

	// 1. Show a simple window
	// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
	{
		static float f = 0.0f;
		ImGui::Text("Hello, world!");
		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
		ImGui::ColorEdit3("clear color", reinterpret_cast<float*>(&clear_col));
		if (ImGui::Button("Test Window")) show_test_window ^= 1;
		//SF TODO ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		
		// Only display this on remote connection (associated texture note created locally)		
		if(!gCustomTextureView && ImGui::Button("Send Texture") )
			CustomTextureCreate();
		else if(gCustomTextureView && ImGui::Button("Remove Texture") )
			CustomTextureDestroy();
			
		// Note: Test drawing with invalid texture (making sure netImgui App handles it) 
		// Avoid drawing with invalid texture if will be displayed locally (not handled by 'ImGui_ImplDX11_RenderDrawData' example code)
		if( gCustomTextureView || (NetImgui::IsRemoteDraw() && !gEnableRemoteMirror) )
		{				
			ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
			ImGui::Image(reinterpret_cast<ImTextureID>(gCustomTextureView), ImVec2(128,32), ImVec2(0, 0), ImVec2(1, 1), tint_col, border_col);
		}		
	}

	// 2. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if (show_test_window)
	{
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);     // Normally user code doesn't need/want to call it because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
		ImGui::ShowDemoWindow(&show_test_window);
	}
}

void Imgui_DrawContentSecondary()
{
	ImGui::Begin("Notice", nullptr, ImGuiWindowFlags_NoTitleBar);
	ImGui::Text("Client connected to remote netImguiApp");
	ImGui::NewLine();
	ImGui::TextWrapped("Can still use Imgui locally, with content different than displayed on netImgui App");
	ImGui::End();	
}

}
