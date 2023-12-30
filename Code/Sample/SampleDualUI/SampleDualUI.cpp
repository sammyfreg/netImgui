//=================================================================================================
// SAMPLE DUAL UI
//-------------------------------------------------------------------------------------------------
// Example of supporting 2 differents ImGui display at the same time. One on the remote server
// and the second in the local client.
// 
// TODO: There's still a bug when switching between DualUI and MirrorUI where the renderer try 
//		 to use an invalid Font Texture. This is a problem with the sample code handling change of
//		 Context/Font and not related to the NetImgui code itself. This needs to be fixed later.
//=================================================================================================

#include <NetImgui_Api.h>
#include "../Common/Sample.h"

//=================================================================================================
// SAMPLE CLASS
//=================================================================================================
class SampleDualUI : public SampleClient_Base
{
public:
						SampleDualUI() : SampleClient_Base("SampleDualUI") {}
	virtual bool		Startup() override;
	virtual void		Shutdown() override;
	virtual ImDrawData* Draw() override;
	virtual bool		UpdateFont(float fontScaleDPI, bool isLocal) override;

protected:
	void				Draw_MainContent();
	void				Draw_LocalContent();
	void				Draw_RemoteContent();
	void				Draw_SharedContent();

	enum class eDisplayMode : int { SingleUI, MirrorUI, DualUI };
	eDisplayMode		mDisplayMode				= eDisplayMode::SingleUI;
	ImGuiContext*		mpContextExtra				= nullptr;	//!< 2nd allocated context used for local draws in DualUI mode (when connected)
	float				mGeneratedFontScaleDPIMain	= 0.f;		//!< Current generated font texture DPI
	float				mGeneratedFontScaleDPIExtra	= 0.f;		//!< Current generated font texture DPI
};

//=================================================================================================
// GET SAMPLE
// Each project must return a valid sample object
//=================================================================================================
SampleClient_Base& GetSample()
{
	static SampleDualUI sample;
	return sample;
}

//=================================================================================================
// STARTUP
//=================================================================================================
bool SampleDualUI::Startup()
{
	if( SampleClient_Base::Startup() )
	{
		mpContextMain	= ImGui::GetCurrentContext();	// Dear ImGui backend already creates a default context
		mpContextExtra	= ImGui::CreateContext();		// Creates an extra context that can be used as a secondary display
		
		// Capture the backend info of the original context and assign it to newly created one
		// Note: This means that only 1 valid Font Texture can exist (1 per backend) and we must
		// thus handle this properly between the 2 fonts.
		// 
		// In you own codebase, this can be made simpler by not relying on a single
		// backend with 1 Font Texture
		void* backendRendererUserData = ImGui::GetIO().BackendRendererUserData;
		void* backendPlatformUserData = ImGui::GetIO().BackendPlatformUserData;
		void* backendLanguageUserData = ImGui::GetIO().BackendLanguageUserData;
		ImGui::SetCurrentContext(mpContextExtra);
		ImGui::GetIO().BackendRendererUserData = backendRendererUserData;
		ImGui::GetIO().BackendPlatformUserData = backendPlatformUserData;
		ImGui::GetIO().BackendLanguageUserData = backendLanguageUserData;
		ImGui::SetCurrentContext(mpContextMain);
		return true;
	}
	return false;
}

//=================================================================================================
// SHUTDOWN
//=================================================================================================
void SampleDualUI::Shutdown()
{
	ImGui::DestroyContext(mpContextExtra);
	ImGui::SetCurrentContext(mpContextMain);
	mpContextMain		= nullptr;
	mpContextExtra	= nullptr;
	SampleClient_Base::Shutdown();
}

//=================================================================================================
// UPDATE FONT
// Special font handling since we can have 2 contexts (local,remote) both needing a font with 
// potentially different DPI scaling, but the sample base class only handle 1 font texture.
// 
// In you own codebase, this can be made simpler by not relying on a single
// backend with 1 Font Texture
//=================================================================================================
bool SampleDualUI::UpdateFont(float fontScaleDPI, bool isLocal)
{
	bool result(false);
	if (isLocal && NetImgui::IsConnected() && mDisplayMode == eDisplayMode::DualUI) {
		ImGui::SetCurrentContext(mpContextExtra);
		mGeneratedFontScaleDPI		= mGeneratedFontScaleDPIExtra;
		result						= SampleClient_Base::UpdateFont(fontScaleDPI, isLocal);
		mGeneratedFontScaleDPIExtra	= mGeneratedFontScaleDPI;
	}
	else {
		ImGui::SetCurrentContext(mpContextMain);
		mGeneratedFontScaleDPI		= mGeneratedFontScaleDPIMain;
		result						= SampleClient_Base::UpdateFont(fontScaleDPI, isLocal);
		mGeneratedFontScaleDPIMain	= mGeneratedFontScaleDPI;
	}
	return result;
}

//=================================================================================================
// DRAW
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
ImDrawData* SampleDualUI::Draw()
{
	//---------------------------------------------------------------------------------------------
	// (0) Update Font Texture
	//---------------------------------------------------------------------------------------------
	// Because we are avoiding changes to original Dear ImGui sample code, the management of
	// the Font texture generation is a little more complicated than it needs to. We only have
	// 1 valid font texture, but 2 possible active Dear ImGui contexts with different font texture. 
	// The context used for local drawing must absolutely have a valid font texture used by the 
	// rendering backend and the remote context only need the font data. 
	// 
	// Here, we detect when the ImGui context has changed between main/extra and make sure there's
	// a valid font texture generated.
	// 
	// This wouldn't be needed if we didn't change the context used for local drawing 
	// (by toggling DualUI mode) or that our sample could easily support 1 font texture per context
	//---------------------------------------------------------------------------------------------
	eDisplayMode displayModeAtStart		= mDisplayMode;
	ImGuiContext* localContextPrevious	= mpContextLocal;
	mpContextLocal						= (NetImgui::IsConnected() && mDisplayMode == eDisplayMode::DualUI) ? mpContextExtra : mpContextMain;
	if ( localContextPrevious != mpContextLocal ){
		float savedScaleMain		= mGeneratedFontScaleDPIMain;
		float saveScaleExtra		= mGeneratedFontScaleDPIExtra;
		bool isExtraContext			= mpContextLocal == mpContextExtra;
		mGeneratedFontScaleDPIMain	= mGeneratedFontScaleDPIExtra = 0.f;
		UpdateFont(isExtraContext ? savedScaleMain : saveScaleExtra, isExtraContext);
	}

	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame
	//---------------------------------------------------------------------------------------------
	//	IMPORTANT NOTE: NetImgui takes over the current context at the time 'ConnectToApp' or 
	//	'ConnectFromApp' is called (inside 'Draw_Connect'), to use for the remote display. 
	//	After a disconnection, it is then restored for local display.
	//
	//	In this sample, we use the existing imgui context for content local content
	//  (when disconnected) and for remote content (when connected). When connected, 
	//	we use a second context for the local content.
	//---------------------------------------------------------------------------------------------
	ImGui::SetCurrentContext(mpContextMain);
	if( NetImgui::NewFrame(true) )
	{
		//-----------------------------------------------------------------------------------------
		// (2) Draw ImGui Content
		//-----------------------------------------------------------------------------------------
		// This is the main content that we are always displaying. When connected, it will appear
		// on the remote server, otherwise will appear in the local window
		SampleClient_Base::Draw_Connect(); //Note: Connection to remote server done in there

		ImGui::SetNextWindowPos(ImVec2(32,48), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(400,500), ImGuiCond_Once);
		if (ImGui::Begin("SampleDualUI", nullptr))
		{	
			Draw_MainContent();
			Draw_SharedContent();
			Draw_LocalContent();
			Draw_RemoteContent();
			ImGui::End();
		}

		//-----------------------------------------------------------------------------------------
		// (3) Finish the frame, preparing the drawing data and...
		// (3a) Send the data to the netImGui server when connected
		//-----------------------------------------------------------------------------------------
		NetImgui::EndFrame();
	}

	//---------------------------------------------------------------------------------------------
	// (4) Draw a second UI for local display, when connected and display mode is 'Independent'
	//---------------------------------------------------------------------------------------------
	if( NetImgui::IsConnected() && displayModeAtStart == eDisplayMode::DualUI )
	{
		ImGui::SetCurrentContext(mpContextExtra);
		ImGui::NewFrame();
		ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_Once);
		if( ImGui::Begin("Extra Local Window") )
		{
			ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Local Window");
			ImGui::TextWrapped("Demonstrate a window content displayed locally while we have some distinct content displayed remotely.");
			Draw_SharedContent();
			Draw_LocalContent();
			// Demonstration: Even though this is called, the method test where we are currently
			// drawing (local or remote) and will not draw anything unless 'remote' is detected
			Draw_RemoteContent();
		}
		ImGui::End();
		ImGui::Render();
	}

	// Makes sure the window size is kept in sync between the both context. 
	// Because the sample doesn't keep track of the current window size, we just makes sure both 
	// context have the last valid value. Otherwise, only the context active during the last resize 
	// window message, will know about the right window size.
	// This is not needed if your own engine already update the display size every frame 
	ImVec2 localDisplaySize		= ImGui::GetIO().DisplaySize;
	ImGui::SetCurrentContext(mpContextLocal == mpContextExtra ? mpContextMain : mpContextExtra);
	ImGui::GetIO().DisplaySize	= localDisplaySize;
	
	// Note: Makes sure the context meant for local drawing is always left active
	//		 so the window input and resizing can still be handled properly. In this sample, 
	//		 we do not draw outside of this method, so the main context won't be missing any draws.
	//		 In your own code, it should be the context you want to receive the default ImGui drawing commands.
	ImGui::SetCurrentContext(mpContextLocal);
	return NetImgui::IsConnected() && displayModeAtStart == eDisplayMode::SingleUI ? nullptr : ImGui::GetDrawData();;
} 

//=================================================================================================
// DRAW LOCAL CONTENT
//=================================================================================================
void SampleDualUI::Draw_LocalContent()
{	
	if( !NetImgui::IsDrawingRemote())
	{
		ImGui::NewLine();
		ImGui::TextWrapped("This text is only displayed locally.");
		if( NetImgui::IsConnected() )
		{
			if (ImGui::Button("Disconnect"))
			{
				NetImgui::Disconnect();
			}
		}
		ImGui::NewLine();
	}
}

//=================================================================================================
// DRAW REMOTE CONTENT
//=================================================================================================
void SampleDualUI::Draw_RemoteContent()
{
	if( NetImgui::IsDrawingRemote() )
	{
		ImGui::NewLine();
		ImGui::TextWrapped("This text is only displayed remotely (unless mirroring the output locally).");
	}
}

//=================================================================================================
// DRAW MAIN CONTENT
//=================================================================================================
void SampleDualUI::Draw_MainContent()
{
	ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Example of local/remote Imgui content");
	ImGui::TextWrapped("Demonstrate the ability of simultaneously displaying distinct content locally and remotely.");
	ImGui::NewLine();
	if (NetImgui::IsConnected())
	{
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Dual Display Mode:");
		ImGui::RadioButton("Single UI", reinterpret_cast<int*>(&mDisplayMode), static_cast<int>(eDisplayMode::SingleUI)); ImGui::SameLine();
		ImGui::RadioButton("Mirror UI", reinterpret_cast<int*>(&mDisplayMode), static_cast<int>(eDisplayMode::MirrorUI)); ImGui::SameLine();
		ImGui::RadioButton("Dual UI", reinterpret_cast<int*>(&mDisplayMode), static_cast<int>(eDisplayMode::DualUI));
		
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f,0.75f,0.75f,1.f));
		ImGui::TextUnformatted("When connected...");
		ImGui::NewLine(); ImGui::SameLine(40.f);
		switch (mDisplayMode) {
		case eDisplayMode::SingleUI: ImGui::TextWrapped("Content is controlled and displayed remotely only."); break;
		case eDisplayMode::MirrorUI: ImGui::TextWrapped("Content is controlled and displayed remotely and also displayed locally.\n(Note: This is for a demonstration, not too useful in reality)"); break;
		case eDisplayMode::DualUI: ImGui::TextWrapped("Content is controlled and displayed remotely and locally, using 2 distincts Dear ImGui contexts."); break;
		}
		ImGui::PopStyleColor();
	}
}

//=================================================================================================
// DRAW SHARED CONTENT
//=================================================================================================
void SampleDualUI::Draw_SharedContent()
{
	ImGui::NewLine();
	ImGui::Text("Elapsed time : %02i:%02i", static_cast<int>(ImGui::GetTime())/60, static_cast<int>(ImGui::GetTime())%60);
	ImGui::Text("Delta time   : %07.04fs", ImGui::GetIO().DeltaTime );
	ImGui::Text("Drawing      : %s", NetImgui::IsDrawingRemote() ? "Remotely" : "Locally" );
}
