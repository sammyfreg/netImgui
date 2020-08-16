//=================================================================================================
// SAMPLE DUAL UI
//-------------------------------------------------------------------------------------------------
// Example of supporting 2 differents ImGui display at the same time. One on the remote server
// and the second in the local client.
//=================================================================================================

#include <NetImgui_Api.h>
#include <array>
#include "..\Common\Sample.h"
#include "..\Common\WarningDisable.h"

// Methods declared in main.cpp, extern declare to avoid having to include 'd3d11.h' here
extern void TextureCreate(const uint8_t* pPixelData, uint32_t width, uint32_t height, void*& pTextureViewOut);
extern void TextureDestroy(void*& pTextureView);

namespace SampleClient
{
static void*	gDefaultEmptyTexture = nullptr;
static void*	gCustomTextureView[static_cast<int>(NetImgui::eTexFormat::kTexFmt_Count)];
const char*		gTextureFormatName[static_cast<int>(NetImgui::eTexFormat::kTexFmt_Count)] = { "R8", "RGBA8" };

//=================================================================================================
//
//=================================================================================================
void CustomTextureCreate(NetImgui::eTexFormat eTexFmt)
{
	constexpr uint32_t BytePerPixelMax = 4;
	constexpr uint32_t Width = 8;
	constexpr uint32_t Height = 8;
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
	case NetImgui::eTexFormat::kTexFmt_Invalid: assert(0); break;
	}
	TextureCreate(pixelData, Width, Height, gCustomTextureView[static_cast<int>(eTexFmt)]);
	NetImgui::SendDataTexture(reinterpret_cast<uint64_t>(gCustomTextureView[static_cast<int>(eTexFmt)]), pixelData, Width, Height, eTexFmt);
}

//=================================================================================================
//
//=================================================================================================
void CustomTextureDestroy(NetImgui::eTexFormat eTexFmt)
{
	if (gCustomTextureView[static_cast<int>(eTexFmt)])
	{
		NetImgui::SendDataTexture(reinterpret_cast<uint64_t>(gCustomTextureView[static_cast<int>(eTexFmt)]), nullptr, 0, 0, NetImgui::eTexFormat::kTexFmt_Invalid);
		TextureDestroy(gCustomTextureView[static_cast<int>(eTexFmt)]);
	}
}

//=================================================================================================
// HELPER FUNCTION
//
// Help start a new ImGui drawing frame, by calling the appropriate function, based on whether 
// we are connected to netImgui server or not.
//
// This function is intended when user intend to display ImGui either locally or remotely, 
// but not both.
//
// Returns True when we need to draw a new ImGui frame 
// (netImgui doesn't expect new content every frame)
//=================================================================================================
bool NetImguiHelper_NewFrame()
{
	if (NetImgui::IsConnected())
	{
		return NetImgui::NewFrame();
	}
	ImGui::NewFrame();
	return true;
}

//=================================================================================================
// HELPER FUNCTION
//
// Help end a ImGui drawing frame, by calling the appropriate function, based on whether we are
// connected to netImgui server or not.
//
// This function is intended when user intend to display ImGui either locally or remotely, 
// but not both.
//
// Returns True when we need render this new ImGui frame locally
//=================================================================================================
bool NetImguiHelper_EndFrame()
{
	if (NetImgui::IsConnected())
	{
		if (NetImgui::IsRemoteDraw())
			NetImgui::EndFrame();

		return false;
	}

	ImGui::Render();
	return true;
}

//=================================================================================================
//
//=================================================================================================
bool Client_Startup()
{
	if (!NetImgui::Startup())
		return false;

	std::array<uint8_t, 8*8*4> pixelData;
	pixelData.fill(0);

	TextureCreate(pixelData.data(), 8, 8, gDefaultEmptyTexture);
	NetImgui::SendDataTexture(reinterpret_cast<uint64_t>(gDefaultEmptyTexture), pixelData.data(), 8, 8, NetImgui::eTexFormat::kTexFmtRGBA8);

	// Can have more ImGui initialization here, like loading extra fonts.
	// ...

	return true;
}

//=================================================================================================
//
//=================================================================================================
void Client_Shutdown()
{
	NetImgui::SendDataTexture(reinterpret_cast<uint64_t>(gDefaultEmptyTexture), nullptr, 0, 0, NetImgui::eTexFormat::kTexFmt_Invalid);
	TextureDestroy(gDefaultEmptyTexture);
	NetImgui::Shutdown();
}

//=================================================================================================
// Added a call to this function in 'ImGui_ImplDX11_CreateFontsTexture()', allowing us to 
// forward the Font Texture information to netImgui.
//=================================================================================================
void Client_AddFontTexture(uint64_t texId, void* pData, uint16_t width, uint16_t height)
{
	NetImgui::SendDataTexture(texId, pData, width, height, NetImgui::eTexFormat::kTexFmtRGBA8);
}

//=================================================================================================
//
//=================================================================================================
void Client_Draw_Connection()
{
	if( NetImgui::IsConnected())
	{
		if( ImGui::Button("Disconnect", ImVec2(120,0)) )
		{
			NetImgui::Disconnect();
		}
	}
	else if( NetImgui::IsConnectionPending() )
	{
		ImGui::Text("Waiting for connection");
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			NetImgui::Disconnect();
		}
	}
	else
	{
		ImGui::TextWrapped("Connect with netImgui server on the same PC, using default port.");
		ImGui::NewLine();
		if( ImGui::Button("Connect To", ImVec2(120,0)) )
		{
			NetImgui::ConnectToApp("SampleDualUI", "localhost", NetImgui::kDefaultServerPort);

		}
		ImGui::SameLine(0.f, 16.f);
		ImGui::Text("Or");
		ImGui::SameLine(0.f, 16.f);
		if (ImGui::Button("Connect Wait", ImVec2(120, 0)))
		{
			NetImgui::ConnectFromApp("SampleDualUI", NetImgui::kDefaultClientPort);
		}
	}
		
}

//=================================================================================================
//
//=================================================================================================
void Client_Draw_RemoteContent()
{
	ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Example of local/remote ImGui content");
	if (ImGui::Button("Disconnect", ImVec2(120, 0)))
	{
		NetImgui::Disconnect();
	}
	ImGui::NewLine();
	ImGui::TextWrapped("And here we have some content displayed only remotely.");
}

//=================================================================================================
//
//=================================================================================================
void Client_Draw_SharedContent()
{
	ImGui::NewLine();
	ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Example of 'IsRemoteDraw()'");
	ImGui::TextWrapped("Demonstration of using 'NetImgui::IsRemoteDraw()' in ImgGui drawing code, to know the current UI output (local/remote) and modify the content accordingly.");
	ImGui::NewLine();
	if( NetImgui::IsRemoteDraw() )
	{
		ImGui::Text("We are drawing : Remotely.");
	}
	else
	{
		ImGui::Text("We are drawing : Locally.");
	}
}
	
//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
ImDrawData* Client_Draw()
{
	//---------------------------------------------------------------------------------------------
	// (1) Prepare a new Frame
	// Note: When remote drawing, it is possible for 'NetImgui::NewFrame()' to return false, 
	//		 in which case we should just skip refreshing the ImGui content.
	//---------------------------------------------------------------------------------------------
	if (NetImguiHelper_NewFrame())
	{
		//-----------------------------------------------------------------------------------------
		// (2) Draw ImGui Content 		
		//-----------------------------------------------------------------------------------------
		ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_Once);
		if (ImGui::Begin("Sample Textures", nullptr))
		{
			const ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			const ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);

			ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Demonstration of textures usage with netImgui.");
			Client_Draw_Connection();
			ImGui::Separator();
			ImGui::NewLine();
			
			ImGui::TextWrapped("Note: Textures properly displayed on netImgui server only, since the original 'Dear ImGui' sample code is limited to the RGBA format.");
			ImGui::NewLine();
			for (int i = 0; i < static_cast<int>(NetImgui::eTexFormat::kTexFmt_Count); ++i)
			{
				// Only display this on remote connection (associated texture note created locally)
				ImGui::PushID(i);				
				if (!gCustomTextureView[i] && ImGui::Button("Texture Create", ImVec2(140,0)))
					CustomTextureCreate(static_cast<NetImgui::eTexFormat>(i));
				else if (gCustomTextureView[i] && ImGui::Button("Texture Destroy", ImVec2(140,0)))
					CustomTextureDestroy(static_cast<NetImgui::eTexFormat>(i));
				ImGui::SameLine();
				ImGui::Text("(%s)", gTextureFormatName[i]);	

				// Note: Test drawing with invalid texture (making sure netImgui App handles it) 
				// Avoid drawing with invalid texture if will be displayed locally (not handled by 'ImGui_ImplDX11_RenderDrawData' example code)				
				{					
					ImGui::SameLine(220.f);
					void* pTexture = gCustomTextureView[i] ? gCustomTextureView[i] : gDefaultEmptyTexture;
					ImGui::Image(reinterpret_cast<ImTextureID>(pTexture), ImVec2(128, 64), ImVec2(0, 0), ImVec2(1, 1), tint_col, border_col);
				}
				
				ImGui::PopID();
			}

			if( NetImgui::IsRemoteDraw() )
			{
				ImGui::NewLine();
				ImGui::TextWrapped("Note: Invalid textures handled by netImgui server by using a default invisible texture.");
				ImGui::NewLine();
				ImGui::TextUnformatted("Invalid texture");
				ImGui::SameLine(220.f);
				ImGui::Image(reinterpret_cast<ImTextureID>(0x12345678), ImVec2(128, 64), ImVec2(0, 0), ImVec2(1, 1), tint_col, border_col);
			}
		}
		ImGui::End();

		//-----------------------------------------------------------------------------------------
		// (3a) Prepare the data for rendering locally (when not connected), or...
		// (3b) Send the data to the netImGui server (when connected)
		//-----------------------------------------------------------------------------------------
		if (NetImguiHelper_EndFrame())
		{
			//-----------------------------------------------------------------------------------------
			// (4a) Let the local DX11 renderer know about the UI to draw (when not connected)
			//		Could also rely on '!NetImgui::IsRemoteDraw()' instead of return value of 
			//		'NetImguiHelper_EndFrame()'
			//-----------------------------------------------------------------------------------------
			return ImGui::GetDrawData();
		}
	}

	//---------------------------------------------------------------------------------------------
	// (4b) Render nothing locally (when connected)
	//---------------------------------------------------------------------------------------------
	return nullptr;
}

} // namespace SampleClient

