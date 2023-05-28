#include "NetImguiServer_App.h"
#include "NetImguiServer_Config.h"
#include "NetImguiServer_Network.h"
#include "NetImguiServer_UI.h"
#include "NetImguiServer_RemoteClient.h"
#include "Fonts/Roboto_Medium.cpp"
#include <algorithm>

namespace NetImguiServer { namespace App
{

constexpr uint32_t		kClientCountMax			= 32;	//! @sammyfreg todo: support unlimited client count
static ServerTexture	gEmptyTexture;

bool Startup(const char* CmdLine)
{	
	//---------------------------------------------------------------------------------------------
	// Load Settings savefile and parse for auto connect commandline option
	//---------------------------------------------------------------------------------------------	
	NetImguiServer::Config::Client::LoadAll();
	AddClientConfigFromString(CmdLine, true);
	
	//---------------------------------------------------------------------------------------------
    // Perform application initialization:
	//---------------------------------------------------------------------------------------------
	if (RemoteClient::Client::Startup(kClientCountMax) &&
		NetImguiServer::Network::Startup() &&
		NetImguiServer::UI::Startup())
	{
		uint8_t EmptyPixels[8*8];
		memset(EmptyPixels, 0, sizeof(EmptyPixels));
		NetImguiServer::App::HAL_CreateTexture(8, 8, NetImgui::eTexFormat::kTexFmtA8, EmptyPixels, gEmptyTexture);
	
		//-----------------------------------------------------------------------------------------
		// Using a different default font (provided with Dear ImGui)
		//-----------------------------------------------------------------------------------------
		ImFontConfig Config;
		NetImgui::Internal::StringCopy(Config.Name, "Roboto Medium, 16px");
		if( !ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(Roboto_Medium_compressed_data, Roboto_Medium_compressed_size, 16.f, &Config) ){
			ImGui::GetIO().Fonts->AddFontDefault();
		}

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
	NetImguiServer::App::HAL_DestroyTexture(gEmptyTexture);
	NetImguiServer::Config::Client::Clear();
	RemoteClient::Client::Shutdown();
	HAL_Shutdown();
}

void DestroyDrawData(ImDrawData*& pDrawData)
{
	if( pDrawData )
	{
		if( pDrawData->CmdLists )
		{
			NetImgui::Internal::netImguiDelete( pDrawData->CmdLists[0] );
			NetImgui::Internal::netImguiDelete( pDrawData->CmdLists );
		}
		NetImgui::Internal::netImguiDeleteSafe( pDrawData );
	}
}

//=================================================================================================
// INIT CLIENT CONFIG FROM STRING
// Take a commandline string, and create a ClientConfig from it.
// Simple format of (Hostname);(HostPort)
bool AddClientConfigFromString(const char* string, bool transient)
//=================================================================================================
{
	NetImguiServer::Config::Client cmdlineClient;
	const char* zEntryStart		= string;
	const char* zEntryCur		= string;
	int paramIndex				= 0;
	cmdlineClient.mTransient	= transient;
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
			client.mpBGContext					= ImGui::CreateContext(ImGui::GetIO().Fonts);
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
				ImDrawData* pDrawData = client.GetImguiDrawData(gEmptyTexture.mpHAL_Texture);
				if( pDrawData )
				{
					DrawClientBackground(client);
					HAL_RenderDrawData(client, pDrawData);
				}
			}
		}
	}
}

//=================================================================================================
bool CreateTexture_Default(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize)
//=================================================================================================
{	
	IM_UNUSED(cmdTexture);
	IM_UNUSED(customDataSize);

	auto eTexFmt = static_cast<NetImgui::eTexFormat>(cmdTexture.mFormat);
	if( eTexFmt < NetImgui::eTexFormat::kTexFmtCustom ){
		NetImguiServer::App::HAL_CreateTexture(cmdTexture.mWidth, cmdTexture.mHeight, eTexFmt, cmdTexture.mpTextureData.Get(), serverTexture);
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
	
	if( serverTexture.mpHAL_Texture ){
		NetImguiServer::App::HAL_DestroyTexture(serverTexture);
	}
	return true;
}

//=================================================================================================
bool CreateTexture(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize)
//=================================================================================================
{
	// Default behavior is to always destroy then re-create the texture
	// But this can be changed inside the DestroyTexture_Custom function
	if(	(serverTexture.mpHAL_Texture != nullptr) ){
		DestroyTexture(serverTexture, cmdTexture, customDataSize);
	}

	if(	CreateTexture_Custom(serverTexture, cmdTexture, customDataSize)		||
		CreateTexture_Default(serverTexture, cmdTexture, customDataSize)	)
	{
		serverTexture.mImguiId	= cmdTexture.mTextureId;
		serverTexture.mFormat	= cmdTexture.mFormat;
		serverTexture.mSize[0]	= cmdTexture.mWidth;
		serverTexture.mSize[1]	= cmdTexture.mHeight;
		return true;	
	}
	return false;
}
//=================================================================================================
void DestroyTexture(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize)
//=================================================================================================
{
	if ( DestroyTexture_Custom(serverTexture, cmdTexture, customDataSize) == false ) {
		DestroyTexture_Default(serverTexture, cmdTexture, customDataSize);
	}
}
}} // namespace NetImguiServer { namespace App
