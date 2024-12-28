#include "NetImguiServer_App.h"
#include "NetImguiServer_Config.h"
#include "NetImguiServer_Network.h"
#include "NetImguiServer_UI.h"
#include "NetImguiServer_RemoteClient.h"
#include "Fonts/Roboto_Medium.cpp"

namespace NetImguiServer { namespace App
{

constexpr uint32_t			kClientCountMax			= 32;	//! @sammyfreg todo: support unlimited client count
static ImTextureData 		gEmptyTexture;
std::atomic<ServerTexture*>	gPendingTextureDelete(nullptr);

bool Startup(const char* CmdLine)
{	
	//---------------------------------------------------------------------------------------------
	// Load Settings savefile and parse for auto connect commandline option
	//---------------------------------------------------------------------------------------------	
	NetImguiServer::Config::Client::LoadAll();
	AddTransientClientConfigFromString(CmdLine);
	
	//---------------------------------------------------------------------------------------------
    // Perform application initialization:
	//---------------------------------------------------------------------------------------------
	if (RemoteClient::Client::Startup(kClientCountMax) &&
		NetImguiServer::Network::Startup() &&
		NetImguiServer::UI::Startup())
	{
		gEmptyTexture.Create(ImTextureFormat::ImTextureFormat_RGBA32, 8, 8);
		gEmptyTexture.Status = ImTextureStatus_Destroyed;
		//SF uint8_t EmptyPixels[8*8];
		//memset(EmptyPixels, 0, sizeof(EmptyPixels));
		//NetImguiServer::App::HAL_CreateTexture(8, 8, NetImgui::eTexFormat::kTexFmtA8, EmptyPixels, gEmptyTexture);
		
		LoadFonts();

		ImGui::GetIO().IniFilename	= nullptr;	// Disable server ImGui ini settings (not really needed, and avoid imgui.ini filename conflicts)
		ImGui::GetIO().LogFilename	= nullptr; 
		return HAL_Startup(CmdLine);
	}
	return false;
}

void Shutdown()
{
	NetImguiServer::Network::Shutdown();
	NetImguiServer::UI::Shutdown();
	//SF NetImguiServer::App::HAL_DestroyTexture(gEmptyTexture);
	NetImguiServer::Config::Client::Clear();
	RemoteClient::Client::Shutdown();
	HAL_Shutdown();
}

//=================================================================================================
// INIT CLIENT CONFIG FROM STRING
// Take a commandline string, and create a ClientConfig from it.
// Simple format of (Hostname);(HostPort)
bool AddTransientClientConfigFromString(const char* string)
//=================================================================================================
{
	NetImguiServer::Config::Client cmdlineClient;
	const char* zEntryStart		= string;
	const char* zEntryCur		= string;
	int paramIndex				= 0;
	cmdlineClient.mConfigType	= NetImguiServer::Config::Client::eConfigType::Transient;
	NetImgui::Internal::StringCopy(cmdlineClient.mClientName, "Commandline");

	while( *zEntryCur != 0 )
	{
		zEntryCur++;
		// Skip commandline preamble holding path to executable
		if (*zEntryCur == ' ' && *(zEntryCur+1) != 0)
		{
			zEntryStart = zEntryCur + 1;
		}
		if( (*zEntryCur == ';' || *zEntryCur == 0) )
		{
			if (paramIndex == 0)
				NetImgui::Internal::StringCopy(cmdlineClient.mHostName, zEntryStart, zEntryCur-zEntryStart);
			else if (paramIndex == 1)
				cmdlineClient.mHostPort = static_cast<uint32_t>(atoi(zEntryStart));

			cmdlineClient.mConnectAuto	= paramIndex >= 1;	//Mark valid for connexion as soon as we have a HostAddress
			zEntryStart					= zEntryCur + 1;
			paramIndex++;
		}
	}

	if (cmdlineClient.mConnectAuto) {
		NetImguiServer::Config::Client::SetConfig(cmdlineClient);
		return true;
	}

	return false;
}

//=================================================================================================
// DRAW CLIENT BACKGROUND
// Create a separate Dear ImGui drawing context, to generate a commandlist that will fill the
// RenderTarget with a specific background picture
void DrawClientBackground(RemoteClient::Client& client)
//=================================================================================================
{
	NetImgui::Internal::CmdBackground* pBGUpdate = client.mPendingBackgroundIn.Release();
	if (pBGUpdate) {
		client.mBGSettings		= *pBGUpdate;
		client.mBGNeedUpdate	= true;
		netImguiDeleteSafe(pBGUpdate);
	}

	if (client.mBGNeedUpdate) 
	{
		if( client.mpBGContext == nullptr )
		{
		#if 0 //SF Broken font reuse
			client.mpBGContext					= ImGui::CreateContext(ImGui::GetIO().Fonts);
		#else
			client.mpBGContext		= ImGui::CreateContext();
			const ImGuiIO& ServerIO = ImGui::GetIO();
			
			NetImgui::Internal::ScopedImguiContext scopedCtx(client.mpBGContext);
			ImGui::GetIO().BackendRendererUserData 	= ServerIO.BackendRendererUserData;
			ImGui::GetIO().BackendRendererName 		= ServerIO.BackendRendererName;
			ImGui::GetIO().ConfigFlags				= ServerIO.ConfigFlags;
			ImGui::GetIO().BackendFlags				= ServerIO.BackendFlags;
			ImGui::GetIO().Fonts->AddFontDefault();
		#endif
			client.mpBGContext->IO.DeltaTime	= 1/30.f;
			client.mpBGContext->IO.IniFilename	= nullptr;	// Disable server ImGui ini settings (not really needed, and avoid imgui.ini filename conflicts)
			client.mpBGContext->IO.LogFilename	= nullptr; 
		}

		NetImgui::Internal::ScopedImguiContext scopedCtx(client.mpBGContext);
		ImGui::GetIO().DisplaySize = ImVec2(client.mAreaSizeX,client.mAreaSizeY);
		ImGui::NewFrame();
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(client.mAreaSizeX,client.mAreaSizeY));
		ImGui::Begin("Background", nullptr, ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoSavedSettings);
		// Look for the desired texture (and use default if not found)
		auto texIt						= client.mTextureTable.find(client.mBGSettings.mTextureId);
		const ServerTexture* pTexture	= texIt == client.mTextureTable.end() ? &UI::GetBackgroundTexture() : &texIt->second;
		UI::DrawCenteredBackground(*pTexture, ImVec4(client.mBGSettings.mTextureTint[0],client.mBGSettings.mTextureTint[1],client.mBGSettings.mTextureTint[2],client.mBGSettings.mTextureTint[3]));
		ImGui::End();
		ImGui::Render();
		client.mBGNeedUpdate = false;
	}
}

//=================================================================================================
// UPDATE REMOTE CONTENT
// Create a render target for each connected remote client once, and update it every frame
// with the last drawing commands received from it. This Render Target will then be used
// normally as the background image of each client window renderered by this Server
void UpdateRemoteContent()
//=================================================================================================
{
	if( gEmptyTexture.Status == ImTextureStatus_Destroyed ){
		gEmptyTexture.Status = ImTextureStatus_WantCreate;
		HAL_UpdateTexture(&gEmptyTexture);
	}
	for (uint32_t i(0); i < RemoteClient::Client::GetCountMax(); ++i)
	{
		RemoteClient::Client& client = RemoteClient::Client::Get(i);
		if( client.mbIsConnected )
		{
			if (client.mbIsReleased) {
				client.Uninitialize();
			}
			else if( client.mbIsVisible ){
				client.ProcessPendingTextures();

				// Update the RenderTarget destination of each client, of size was updated
				if (client.mAreaSizeX > 0 && client.mAreaSizeY > 0 && (!client.mpHAL_AreaRT || client.mAreaRTSizeX != client.mAreaSizeX || client.mAreaRTSizeY != client.mAreaSizeY))
				{
					if (HAL_CreateRenderTarget(client.mAreaSizeX, client.mAreaSizeY, client.mpHAL_AreaRT, client.mpHAL_AreaTexture))
					{
						client.mAreaRTSizeX		= client.mAreaSizeX;
						client.mAreaRTSizeY		= client.mAreaSizeY;
						client.mLastUpdateTime	= std::chrono::steady_clock::now() - std::chrono::hours(1); // Will redraw the client
						client.mBGNeedUpdate	= true;
					}
				}

				// Render the remote results
				ImDrawData* pDrawData = client.GetImguiDrawData(gEmptyTexture.BackendTexID);
				if( pDrawData )
				{
					DrawClientBackground(client);
					HAL_RenderDrawData(client, pDrawData);
				}
			}
		}
	}
	CompleteHALTextureDestroy();
}

//=================================================================================================
bool CreateTexture_Default(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize)
//=================================================================================================
{	
	IM_UNUSED(cmdTexture);
	IM_UNUSED(customDataSize);

	//auto eTexFmt = static_cast<NetImgui::eTexFormat>(cmdTexture.mFormat);
	if( !serverTexture.mIsCustom )
	{
		if( cmdTexture.mpTextureData.Get() != nullptr )
		{
			auto NetImguiFmt			= static_cast<NetImgui::eTexFormat>(cmdTexture.mFormat);
			auto ImGuiFormat			= (cmdTexture.mFormat == NetImgui::eTexFormat::kTexFmtA8) ? ImTextureFormat::ImTextureFormat_Alpha8 : ImTextureFormat::ImTextureFormat_RGBA32;
			IM_UNUSED(NetImguiFmt);
			serverTexture.mIsUpdatable	= cmdTexture.mUpdatable;
			serverTexture.mTexData.Create(ImGuiFormat, cmdTexture.mWidth, cmdTexture.mHeight);
			IM_ASSERT(serverTexture.mTexData.BytesPerPixel*8 == static_cast<int>(NetImgui::GetTexture_BitsPerPixel(NetImguiFmt)));
			memcpy(serverTexture.mTexData.Pixels, cmdTexture.mpTextureData.Get(), customDataSize);

			serverTexture.mTexData.Status		= ImTextureStatus_WantCreate;

			serverTexture.mTexData.BackendTexID = 0; //SF TMP HACK (leak texture, until update support added)
			serverTexture.mTexData.BackendUserData = 0; //SF TMP HACK (leak)
			HAL_UpdateTexture(&serverTexture.mTexData);
		}
		//SF TODO free texture data
		/*
		//serverTexture.mTexData.BackendTexID			= 0;
		//serverTexture.mTexData.Width				= cmdTexture.mWidth;
		//serverTexture.mTexData.Height				= cmdTexture.mHeight;
		//serverTexture.mTexData.Format				= cmdTexture.mFormat == NetImgui::eTexFormat::kTexFmtA8 ? ImTextureFormat::ImTextureFormat_Alpha8 : ImTextureFormat::ImTextureFormat_RGBA32;
		//serverTexture.mTexData.Status				= ImTextureStatus::ImTextureStatus_WantCreate;
		
		serverTexture.mTexData.UpdatesMerged.X0		= cmdTexture.mUpdateAllCoords[0];
		serverTexture.mTexData.UpdatesMerged.X0		= cmdTexture.mUpdateAllCoords[1];
		serverTexture.mTexData.UpdatesMerged.X1		= cmdTexture.mUpdateAllCoords[2];
		serverTexture.mTexData.UpdatesMerged.Y1		= cmdTexture.mUpdateAllCoords[3];
		serverTexture.mTexData.Updates[0].X0		= cmdTexture.mUpdateCoords[0][0];
		serverTexture.mTexData.Updates[0].Y0		= cmdTexture.mUpdateCoords[0][1];
		serverTexture.mTexData.Updates[0].X1		= cmdTexture.mUpdateCoords[0][2];
		serverTexture.mTexData.Updates[0].Y1		= cmdTexture.mUpdateCoords[0][3];
		*/
		
		

		return true;
	}
	return false;
}

//=================================================================================================
bool DestroyTexture_Default(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize)
//=================================================================================================
{
	IM_UNUSED(cmdTexture);
	IM_UNUSED(customDataSize);
	
	(void)serverTexture;//SF
	//SF if( serverTexture.mpHAL_Texture ){
	//	EnqueueHALTextureDestroy(serverTexture);
	//}
	return true;
}

//=================================================================================================
bool CreateTexture(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize)
//=================================================================================================
{
	// Default behavior is to always destroy then re-create the texture
	// But this can be changed inside the DestroyTexture_Custom function
	#if 0 //SF TODO
	if(	(serverTexture.mpHAL_Texture != nullptr) )
	{
		DestroyTexture(serverTexture, cmdTexture, customDataSize);
	}
	#endif

	//serverTexture.mTexID._TexUserID	= cmdTexture.mTextureUserId;
	serverTexture.mIsCustom			= cmdTexture.mFormat == NetImgui::eTexFormat::kTexFmtCustom;
	return 	CreateTexture_Custom(serverTexture, cmdTexture, customDataSize) ||
			CreateTexture_Default(serverTexture, cmdTexture, customDataSize);
}
//=================================================================================================
void DestroyTexture(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize)
//=================================================================================================
{
	if ( DestroyTexture_Custom(serverTexture, cmdTexture, customDataSize) == false )
	{
		DestroyTexture_Default(serverTexture, cmdTexture, customDataSize);
	}
}

//=================================================================================================
void EnqueueHALTextureDestroy(ServerTexture& serverTexture)
//=================================================================================================
{
	ServerTexture* pDeleteTexture	= new ServerTexture(serverTexture);
	pDeleteTexture->mpDeleteNext	= gPendingTextureDelete.exchange(pDeleteTexture);
	memset(&serverTexture, 0, sizeof(serverTexture)); // Making sure we don't double free this later
}

//=================================================================================================
void CompleteHALTextureDestroy()
//=================================================================================================
{
	ServerTexture* pTexture = gPendingTextureDelete.exchange(nullptr);
	while(pTexture)
	{
		ServerTexture* pDeleteMe	= pTexture;
		pTexture					= pTexture->mpDeleteNext;
		//SF TODO HAL_DestroyTexture(*pDeleteMe);
		delete pDeleteMe;
	}
}

//-----------------------------------------------------------------------------------------
// Update the Font texture when new monitor DPI is detected
//SF TODO change font setting instead
//-----------------------------------------------------------------------------------------
void UpdateFonts()
{
	static float sGeneratedDPI	= 1.f;
	float currentDPIScale		= NetImguiServer::UI::GetFontDPIScale();
	float scaleDiff				= currentDPIScale - sGeneratedDPI;
	scaleDiff					*= scaleDiff < 0 ? -1.f : 1.f;
	if( scaleDiff >= 0.01f )
	{
		float ratio		= currentDPIScale / sGeneratedDPI;
		sGeneratedDPI	= currentDPIScale;
		for(auto& font : ImGui::GetIO().Fonts->Fonts)
		{
			font->FontSize 	*= ratio;
			for (int cfg_n = 0; cfg_n < font->ConfigDataCount; cfg_n++)
			{
				ImFontConfig* cfg = (ImFontConfig*)(void*)(&font->ConfigData[cfg_n]);
				cfg->SizePixels *= ratio;
			}
			ImFontAtlasBuildReloadFont(font->ContainerAtlas, font);
		}
	}
}
//-----------------------------------------------------------------------------------------
// Initialize all needed fonts by the NetImguiServer application
//-----------------------------------------------------------------------------------------
void LoadFonts()
{
	ImFontConfig fontConfig;
	ImFontAtlas* pFontAtlas = ImGui::GetIO().Fonts;

	// Add Fonts here...
	// Using a different default font (provided with Dear ImGui)
	NetImgui::Internal::StringCopy(fontConfig.Name, "Roboto Medium, 16px");	
	if( !pFontAtlas->AddFontFromMemoryCompressedTTF(Roboto_Medium_compressed_data, Roboto_Medium_compressed_size, 16.f, &fontConfig) ){
		fontConfig.SizePixels = 16.f;
		pFontAtlas->AddFontDefault(&fontConfig);
	}
	
	#if 0 // SF
    // (1)
    io.Fonts->AddFontFromFileTTF("../../../fonts/NotoSans-Regular.ttf", 16.0f * scale);
    {
        //static ImWchar ranges[] = { 0x1, 0x1FFFF, 0 };
        ImFontConfig cfg;
        cfg.OversampleH = cfg.OversampleV = 1;
        cfg.MergeMode = true;
		#ifdef IMGUI_ENABLE_FREETYPE
        cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
		#endif
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguiemj.ttf", 16.0f * scale, &cfg);// , ranges);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/NotoColorEmoji.ttf", 16.0f, &cfg, ranges);
    }
    {
        ImFontConfig cfg;
        cfg.MergeMode = true;
        io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 16.0f * scale, &cfg);
    }

    // (2)
    {
        ImFontConfig cfg_main;
        //cfg.OversampleH = 1;
        ImFont* font_main = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f * scale, &cfg_main);
        IM_UNUSED(font_main);

        ImFontConfig cfg_icon;
        cfg_icon.MergeMode = true;
        cfg_icon.OversampleH = 1;
        ImFont* font_icon = io.Fonts->AddFontFromFileTTF("../../../fonts/fa-solid-900.ttf", 18.0f * scale, &cfg_icon);
        IM_UNUSED(font_icon);
    }

    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 17.0f * scale, nullptr, io.Fonts->GetGlyphRangesJapanese());

    io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f * scale);
    io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f * scale);
    io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f * scale);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f * scale);
	#endif
}

}} // namespace NetImguiServer { namespace App
