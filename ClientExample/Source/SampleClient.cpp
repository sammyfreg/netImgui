#include <chrono>
#include "SampleClient.h"
#include <NetImgui_Api.h>

int				gConnectPort			= NetImgui::kDefaultServerPort;
int				gConnectIP[4]			= {127,0,0,1};

namespace SampleClient
{


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

void Client_Shutdown()
{	
	NetImgui::Shutdown();
}

void Client_AddFontTexture(uint64_t texId, void* pData, uint16_t width, uint16_t height)
{
	NetImgui::SendDataTexture(texId, pData, width, height, NetImgui::kTexFmtRGBA8 );
}

void Client_DrawLocal(ImVec4& clearColorOut)
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
}

// Normal DebugUI rendering
// Can either be displayed locally or sent to remote netImguiApp server if connected
void Client_DrawRemote(ImVec4& clearColorOut)
{		
	if( NetImgui::NewFrame() )
	{		
		SampleClient::Imgui_DrawMainMenu();
		SampleClient::Imgui_DrawContent(clearColorOut);
		NetImgui::EndFrame();
	}
}

void Imgui_DrawMainMenu()
{
	ImGui::BeginMainMenuBar();
	if( NetImgui::IsConnected() )
	{
		if( ImGui::Button("Disconnect") )
			NetImgui::Disconnect();
	}
	else
	{		
		ImGui::Text("IP:");ImGui::SameLine();
		ImGui::PushItemWidth(180); ImGui::DragInt4("##IP",gConnectIP,0,255); ImGui::PopItemWidth(); ImGui::SameLine();
		ImGui::Text("Port:"); ImGui::SameLine();
		ImGui::PushItemWidth(80); ImGui::InputInt("##PORT", &gConnectPort); ImGui::PopItemWidth(); ImGui::SameLine();
		if( ImGui::Button("Connect") )
		{
			unsigned char IpAddress[4] = {(uint8_t)gConnectIP[0], (uint8_t)gConnectIP[1], (uint8_t)gConnectIP[2], (uint8_t)gConnectIP[3] };
			NetImgui::Connect("SampleClientPC", IpAddress, gConnectPort);
		}
			
	}
	ImGui::EndMainMenuBar();
}

void Imgui_DrawContent(ImVec4& clear_col)
{
	// Straight from DearImgui sample
	static bool show_test_window	= true;
	static bool show_another_window	= false;
	// 1. Show a simple window
	// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
	{
		static float f = 0.0f;
		ImGui::Text("Hello, world!");
		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
		ImGui::ColorEdit3("clear color", (float*)&clear_col);
		if (ImGui::Button("Test Window")) show_test_window ^= 1;
		if (ImGui::Button("Another Window")) show_another_window ^= 1;
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}

	// 2. Show another simple window, this time using an explicit Begin/End pair
	if (show_another_window)
	{
		ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiCond_FirstUseEver);
		ImGui::Begin("Another Window", &show_another_window);
		ImGui::Text("Hello");
		ImGui::End();
	}

	// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
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
	ImGui::End();	
}

}
