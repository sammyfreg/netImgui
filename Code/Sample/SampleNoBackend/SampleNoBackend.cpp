//=================================================================================================
// SAMPLE NO BACKEND
//-------------------------------------------------------------------------------------------------
// Demonstration of using Dear ImGui without any Backend support.
// This way, user can use DearImgui without any code needed for drawing / input management,
// since it is all handled by the NetImgui remote server instead. Useful when this code
// is running on hardward without any display and/or convenient input.
// 
// Because we are not using any Backend code, this Sample is a little bit different from 
// the others. All of its code is included in this file (except for Dear ImGui sources)
// and does not rely on some shared sample source file.
// 
// This sample compile both 'Dear Imgui' and 'NetImgui' sources directly 
// (not using their project version in the solution)
// 
// NOTE:	This sample is also use in backward compatibility test with older Dear ImGui versions,
//			making it easier to compile Dear Imgui without any OS specific code (Backends)
// 
// NOTE:	This is also an excellent example of own little is needed to add NetImgui support
//			to a project. It doesn't handle Font DPI regeneration, keeping things simple.
//=================================================================================================

#include <stdio.h>

// By defining 'NETIMGUI_IMPLEMENTATION' before '#include <NetImgui_Api.h>', you can tell the 
// library to pull all source files needed to compile NetImgui here.
//
// It should only be done in 1 source file (to avoid duplicate symbol at link time).
//
// Other location can still include 'NetImgui_Api.h', but without using the define
//
// Note: Another (more conventional) way of compiling 'NetImgui' with your code, 
//		 is to includes its sources file directly in your project. This single header include
//		 approach was added for potential convenience, minimizing changes to a project, but
//		 can prevent code change detection in these included files, when compiling.
#define NETIMGUI_IMPLEMENTATION
#include <NetImgui_Api.h>

#include <chrono>

namespace SampleNoBackend
{

enum eSampleState : uint8_t {
	Start,
	Disconnected,
	Connected,
};

#if NETIMGUI_FONTUPDATE_TEMP_WORKAROUND
// Based on ImGui_ImplDX12_UpdateTexture, but without actual texture handling
void ImGui_ImplDX12_UpdateTexture(ImTextureData* tex)
{
	static uint64_t UniqueTextureID(1);
	if (tex->Status == ImTextureStatus_WantCreate)
    {
        // Create and upload new texture to graphics system
        //IMGUI_DEBUG_LOG("UpdateTexture #%03d: WantCreate %dx%d\n", tex->UniqueID, tex->Width, tex->Height);
        IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == nullptr);
        IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);

        // Store identifiers
        tex->SetTexID((ImTextureID)UniqueTextureID++);
    }

    if (tex->Status == ImTextureStatus_WantCreate || tex->Status == ImTextureStatus_WantUpdates)
    {
        IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);
        tex->SetStatus(ImTextureStatus_OK);
    }

	if (tex->Status == ImTextureStatus_WantDestroy && tex->UnusedFrames >= 2)
	{
		// Clear identifiers and mark as destroyed (in order to allow e.g. calling InvalidateDeviceObjects while running)
		tex->SetTexID(ImTextureID_Invalid);
		tex->SetStatus(ImTextureStatus_Destroyed);
	}
}

// This hook into the PostRender callback to 'update' the ImGui Textures
// Since this is a Sample for no Backend, only assign an ID to new textures
void Client_UpdateTexturesCallback(ImGuiContext*, ImGuiContextHook* hook)
{
	ImVector<ImTextureData*>& Textures = ImGui::GetPlatformIO().Textures;
	for (ImTextureData* tex : Textures)
	{
		if (tex->Status != ImTextureStatus_OK)
		{
			ImGui_ImplDX12_UpdateTexture(tex);
		}
	}
}
#endif //NETIMGUI_FONTUPDATE_TEMP_WORKAROUND

//=================================================================================================
// Initialize the Dear Imgui Context and the NetImgui library
//=================================================================================================
bool Client_Startup()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	io.DisplaySize = ImVec2(8,8);
	io.BackendFlags |= ImGuiBackendFlags_HasGamepad;	// Enable NetImgui Gamepad support
	ImGui::StyleColorsDark();
#if NETIMGUI_FONTUPDATE_TEMP_WORKAROUND
	ImGuiContextHook hookEndframe;
	hookEndframe.HookId		= 0;
	hookEndframe.Type		= ImGuiContextHookType_RenderPost;
	hookEndframe.Callback	= Client_UpdateTexturesCallback;
	hookEndframe.UserData	= nullptr;
	ImGui::AddContextHook(ImGui::GetCurrentContext(), &hookEndframe);
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
#else
	io.Fonts->Build();
	io.Fonts->SetTexID(0);
#endif
	if( !NetImgui::Startup() )
		return false;

	return true;
}

//=================================================================================================
// Release resources
//=================================================================================================
void Client_Shutdown()
{	
	NetImgui::Shutdown();
	ImGui::DestroyContext(ImGui::GetCurrentContext());
}

//=================================================================================================
// Manage connection to NetImguiServer
//=================================================================================================
void Client_Connect(eSampleState& sampleState)
{
	constexpr char zClientName[] = "SampleNoBackend (ImGui " IMGUI_VERSION ")";
	if( sampleState == eSampleState::Start )
	{
		printf("- Connecting to NetImguiServer to (127.0.0.1:%i)... ", NetImgui::kDefaultServerPort);
		NetImgui::ConnectToApp(zClientName, "localhost");
		while( NetImgui::IsConnectionPending() );
		bool bSuccess   = NetImgui::IsConnected();
		sampleState     = bSuccess ? eSampleState::Connected : eSampleState::Disconnected;
		printf(bSuccess ? "Success\n" : "Failed\n");
		if (!bSuccess) {
			printf("- Waiting for a connection from NetImguiServer on port %i... ", NetImgui::kDefaultClientPort);
			NetImgui::ConnectFromApp(zClientName);
			
		}
	}
	else if (sampleState == eSampleState::Disconnected && NetImgui::IsConnected())
	{
		sampleState = eSampleState::Connected;
		printf("CONNECTED\n");
	}
	else if (sampleState == eSampleState::Connected && !NetImgui::IsConnected())
	{
		sampleState = eSampleState::Disconnected;
		printf("DISCONNECTED\n");
		printf("- Waiting for a connection from NetImguiServer on port %i... ", NetImgui::kDefaultClientPort);
		NetImgui::ConnectFromApp(zClientName);
	}
}

//=================================================================================================
// Render all of our Dear ImGui Content (when appropriate)
//=================================================================================================
void Client_Draw(bool& bQuit)
{
	if( NetImgui::IsConnected() && NetImgui::NewFrame(true) ){
	
		ImGui::ShowDemoWindow();

		ImGui::SetNextWindowPos(ImVec2(32,48), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(400,400), ImGuiCond_Once);
		if( ImGui::Begin("Sample No Backend", nullptr) )
		{
			ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Client:");
			ImGui::TextUnformatted(" DearImgui Version: " IMGUI_VERSION);
			ImGui::TextUnformatted(" NetImgui Version:  " NETIMGUI_VERSION);
			ImGui::TextUnformatted(""); ImGui::SameLine();
			bQuit = ImGui::Button("  Quit  ");
			if( ImGui::IsItemHovered() )
				ImGui::SetTooltip("Terminate this sample.");
			ImGui::NewLine();
			ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Description:");
			ImGui::TextWrapped(	"This sample demonstate the ability to use Dear ImGui without any Backend. "
								"Rely instead on NetImgui to remotely handle drawing and inputs. This avoids the need for rendering/input/window management code on the client itself.");
		}
		ImGui::End();
		NetImgui::EndFrame();
	}
}
} // namespace SampleNoBackend

int main(int, char**)
{
	printf("================================================================================\n");
	printf("  NetImgui Sample: No Backend\n");
	printf("================================================================================\n");
	printf(" Demonstrate 'Dear ImGui' + 'NetImgui' for a UI displayed on a remote server.\n");
	printf(" Dear ImGui : " IMGUI_VERSION "\n");
	printf(" NetImgui   : " NETIMGUI_VERSION "\n");
	printf(" ['Ctrl + C' to quit]\n");
	printf("--------------------------------------------------------------------------------\n");
	if ( !SampleNoBackend::Client_Startup() ) {
		printf("Failed initializing NetImgui.");
		SampleNoBackend::Client_Shutdown();
		return -1;
	}

	// Main loop
	bool bQuit									= false;
	SampleNoBackend::eSampleState sampleState	= SampleNoBackend::eSampleState::Start;
	while (!bQuit)
	{
		// Avoids high CPU/GPU usage by releasing this thread until enough time has passed
		static auto sLastTime = std::chrono::steady_clock::now();
		std::chrono::duration<float> elapsedSec	= std::chrono::steady_clock::now() - sLastTime;
		if( elapsedSec.count() < 1.f/120.f ){
			std::this_thread::sleep_for(std::chrono::microseconds(250));
			continue;
		}
		sLastTime = std::chrono::steady_clock::now();
		
		// Sample Update
		SampleNoBackend::Client_Connect(sampleState);
		SampleNoBackend::Client_Draw(bQuit);
	}

	// Cleanup
	SampleNoBackend::Client_Shutdown();
	return 0;
}
