//=================================================================================================
// SAMPLE COMPRESSION
//-------------------------------------------------------------------------------------------------
// Demonstate the benefits of using compression for communications
//=================================================================================================

// Defining this value before '#include <NetImgui_Api.h>', also load all 'NetImgui' client sources
// It should only be done in 1 source file (to avoid duplicate symbol at link time), 
// other location can still include 'NetImgui_Api.h', but without using the define
#define NETIMGUI_IMPLEMENTATION
#include <NetImgui_Api.h>

#include "..\Common\Sample.h"

extern uint64_t gMetric_SentDataCompressed;
extern uint64_t gMetric_SentDataUncompressed;
extern float	gMetric_SentDataTimeUS;

namespace SampleClient
{

//=================================================================================================
//
//=================================================================================================
bool Client_Startup()
{
	if( !NetImgui::Startup() )
		return false;

	// Can have more ImGui initialization here, like loading extra fonts.
	// ...
    
	return true;
}

//=================================================================================================
//
//=================================================================================================
void Client_Shutdown()
{	
	NetImgui::Shutdown();
}

void Client_Draw_ExtraWindowDraw(const char* name, const ImVec2& pos)
{
	ImGui::SetNextWindowPos(pos, ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(400,400), ImGuiCond_Once);
	if( ImGui::Begin(name, nullptr) )
	{
		ImGui::TextWrapped("Basic Window with some text.");
		ImGui::NewLine();
		ImGui::TextWrapped("Drawing more than 1 window to better see compression improvements.");
		ImGui::NewLine();
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Where are we drawing: ");
		ImGui::SameLine();
		ImGui::TextUnformatted(NetImgui::IsDrawingRemote() ? "Remote Draw" : "Local Draw");
		ImGui::NewLine();
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Filler content");
		ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
	}
	ImGui::End();
}

//=================================================================================================
// Imgui drawing when there's a NetImgui connection detected. 
// Draw main compression sample window
//=================================================================================================
void Client_Draw_RemoteDraw()
{
	//if(!NetImgui::IsConnected())
	//	return;

	
}
//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
ImDrawData* Client_Draw()
{
	//---------------------------------------------------------------------------------------------
	// (0) Update the communications statistics
	//---------------------------------------------------------------------------------------------
	static auto sPreviousTime				= std::chrono::steady_clock::now();
	static uint64_t sStatsCompressed		= 0;
	static uint64_t sStatsUncompressed		= 0;
	static float sMetric_CompressedBps		= 0.f;
	static float sMetric_UncompressedBps	= 0.f;
	static float sMetric_RenderTimeUS		= 0.f;
	static float sMetric_FrameDrawTimeUS	= 0.f;
	auto timeDrawStart						= std::chrono::steady_clock::now();
	auto elapsedTime						= timeDrawStart - sPreviousTime;

	if (!NetImgui::IsConnected())
	{
		sMetric_CompressedBps	= sMetric_UncompressedBps	= 0.f;
		sStatsCompressed		= sStatsUncompressed		= 0u;
		gMetric_SentDataTimeUS	= 0.f;
	}
	else if( std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count() >= 250 )
	{
		constexpr uint64_t kHysteresis	= 20; // out of 100
		uint64_t newDataCompressed		= gMetric_SentDataCompressed - sStatsCompressed;
		uint64_t newDataUncompressed	= gMetric_SentDataUncompressed - sStatsUncompressed;
		uint64_t tmMicrosS				= std::chrono::duration_cast<std::chrono::microseconds>(elapsedTime).count();
		uint32_t newDataRcvdBps			= static_cast<uint32_t>(newDataCompressed * 1000000u / tmMicrosS);
		uint32_t newDataSentBps			= static_cast<uint32_t>(newDataUncompressed * 1000000u / tmMicrosS);
		sMetric_CompressedBps			= (sMetric_CompressedBps*(100u-kHysteresis) + newDataRcvdBps*kHysteresis)/100u;
		sMetric_UncompressedBps			= (sMetric_UncompressedBps*(100u-kHysteresis) + newDataSentBps*kHysteresis)/100u;
		sPreviousTime					= std::chrono::steady_clock::now();
		sStatsCompressed				= gMetric_SentDataCompressed;
		sStatsUncompressed				= gMetric_SentDataUncompressed;
	}
	
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
	ClientUtil_ImGuiContent_Common("SampleCompression"); //Note: Connection to remote server done in there
	ImGui::SetNextWindowPos(ImVec2(32,48), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(400,500), ImGuiCond_Once);
	if( ImGui::Begin("Sample Compression", nullptr) )
	{
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Demonstration of NetImgui data compression.");
		ImGui::TextWrapped(	"Show impact of data compression usage."
							"Greatly reduce bandwidth at a near zero cost on the client."
							"When enabled, NetImgui::EndFrame() takes slightlty longuer while data transmission takes slightly less."
							"\n\nCompression is based on sending only the difference from previous frame and is affected by dynamic content (work best with static content).");
		ImGui::NewLine();

		const char* compressionModeNames[]={"Disabled", "Enabled", "Server Setting"};
		const char* compressionModeTips[]={
			"Ignore Server compression setting and force disable compression", 
			"Ignore Server compression setting and force enable compression", 
			"(Default setting)\nUse Server compression Setting. Can be found under 'Settings->Use Compression'"
		};
		int compressionMode = static_cast<int>(NetImgui::GetCompressionMode());
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Compression Mode");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::Combo("##Compression Mode", &compressionMode, compressionModeNames, 3);
		NetImgui::SetCompressionMode(static_cast<NetImgui::eCompressionMode>(compressionMode));
		if( ImGui::BeginChild("Hint", ImVec2(0, 60.f), true) ){
			ImGui::TextWrapped("%s", compressionModeTips[compressionMode]);
			ImGui::EndChild();
		}
		ImGui::NewLine();

		ImGui::TextColored(ImVec4(0.1f, 1.0f, 0.1f, 1), "Performances");
		ImGui::Text(									"Client_Draw() :   %5.03f ms", sMetric_FrameDrawTimeUS/1000.f);
		if( ImGui::IsItemHovered() ) ImGui::SetTooltip("Sample main update function with its Dear ImGui drawing. Help give a scale to the NetImgui cost.");
		ImGui::Text(									"NetImgui process: %5.03f ms", (sMetric_RenderTimeUS+gMetric_SentDataTimeUS)/1000.f);
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), "...EndFrame:      %5.03f ms", sMetric_RenderTimeUS/1000.f);
		if( ImGui::IsItemHovered() ) ImGui::SetTooltip("Includes time for Dear ImGui to generate rendering data, NetImgui to create draw command and compress it.");
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), "...Transmit:      %5.03f ms", gMetric_SentDataTimeUS/1000.f);
		if( ImGui::IsItemHovered() ) ImGui::SetTooltip("Includes time for NetImgui to send the results to the NetImgui server (on communication thread).");
		ImGui::NewLine();

		float compressionRate = NetImgui::IsConnected() ? sMetric_UncompressedBps/sMetric_CompressedBps : 0.f;
		ImGui::TextColored(ImVec4(0.1f, 1.0f, 0.1f, 1), "Bandwidth (Client to Server)");
		ImGui::Text(									"Compression rate:   %7.01fx", compressionRate );
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), "  Compressed data:  %5i KB/s", static_cast<int>(sMetric_CompressedBps/1024.f) );
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), "  Unompressed data: %5i KB/s", static_cast<int>(sMetric_UncompressedBps/1024.f) );
	}
	ImGui::End();

	Client_Draw_ExtraWindowDraw("Extra Window 1", ImVec2(500,48));
	Client_Draw_ExtraWindowDraw("Extra Window 2", ImVec2(600,148));
	Client_Draw_ExtraWindowDraw("Extra Window 3", ImVec2(700,248));
		
	//---------------------------------------------------------------------------------------------
	// (3) Finish the frame
	// Note:	Same note as in (1)
	//---------------------------------------------------------------------------------------------
	{
		constexpr float kHysteresis	= 1.f; // out of 100
		auto timeBeforeRender	= std::chrono::steady_clock::now();
		ImGui::Render();
		auto elapsedRender		= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - timeBeforeRender);
		sMetric_RenderTimeUS	= (sMetric_RenderTimeUS*(100.f-kHysteresis) + static_cast<float>(elapsedRender.count()) / 1000.f * kHysteresis) / 100.f;

		auto elapsedDraw		= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - timeDrawStart);
		sMetric_FrameDrawTimeUS	= (sMetric_FrameDrawTimeUS*(100.f-kHysteresis) + static_cast<float>(elapsedDraw.count())/ 1000.f * kHysteresis) / 100.f;
	}
	//---------------------------------------------------------------------------------------------
	// (4) Return content to draw by local renderer. Stop drawing locally when remote connected
	//---------------------------------------------------------------------------------------------
	return !NetImgui::IsConnected() ? ImGui::GetDrawData() : nullptr;	
}

} // namespace SampleClient
