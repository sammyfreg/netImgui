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

ImDrawData* CreateDrawData(RemoteClient::Client& client)
{
	const NetImgui::Internal::CmdDrawFrame* pCmdDrawFrame = client.TakeDrawFrame();	
	if (!pCmdDrawFrame){
		return nullptr;
	}
	client.mMouseCursor				= static_cast<ImGuiMouseCursor>(pCmdDrawFrame->mMouseCursor);

	ImDrawData* pDrawData			= NetImgui::Internal::netImguiNew<ImDrawData>();
	pDrawData->Valid				= true;    
    pDrawData->CmdListsCount		= 1; // All draws collapsed in same CmdList	
    pDrawData->TotalVtxCount		= static_cast<int>(pCmdDrawFrame->mVerticeCount);
	pDrawData->TotalIdxCount		= static_cast<int>(pCmdDrawFrame->mIndiceCount);
    pDrawData->DisplayPos.x			= pCmdDrawFrame->mDisplayArea[0];
	pDrawData->DisplayPos.y			= pCmdDrawFrame->mDisplayArea[1];
    pDrawData->DisplaySize.x		= pCmdDrawFrame->mDisplayArea[2] - pCmdDrawFrame->mDisplayArea[0];
	pDrawData->DisplaySize.y		= pCmdDrawFrame->mDisplayArea[3] - pCmdDrawFrame->mDisplayArea[1];
    pDrawData->FramebufferScale		= ImVec2(1,1); //! @sammyfreg Currently untested, so force set to 1
    pDrawData->OwnerViewport		= nullptr;
	pDrawData->CmdLists				= NetImgui::Internal::netImguiNew<ImDrawList*>();
	pDrawData->CmdLists[0]			= NetImgui::Internal::netImguiNew<ImDrawList>(nullptr);
	ImDrawList* pCmdList			= pDrawData->CmdLists[0];
	
	// Vertex Buffer Conversion
	pCmdList->VtxBuffer.resize(pCmdDrawFrame->mVerticeCount);
	constexpr float kPosRangeMin	= static_cast<float>(NetImgui::Internal::ImguiVert::kPosRange_Min);
	constexpr float kPosRangeMax	= static_cast<float>(NetImgui::Internal::ImguiVert::kPosRange_Max);
	constexpr float kUVRangeMin		= static_cast<float>(NetImgui::Internal::ImguiVert::kUvRange_Min);
	constexpr float kUVRangeMax		= static_cast<float>(NetImgui::Internal::ImguiVert::kUvRange_Max);
	ImDrawVert* pVertexDst			= &pCmdList->VtxBuffer[0];
	const NetImgui::Internal::ImguiVert* pVertexSrc	= &pCmdDrawFrame->mpVertices[0];
	for (uint32_t i(0); i < pCmdDrawFrame->mVerticeCount; ++i)
	{
		pVertexDst[i].pos.x	= (static_cast<float>(pVertexSrc[i].mPos[0]) * (kPosRangeMax - kPosRangeMin)) / static_cast<float>(0xFFFF) + kPosRangeMin;
		pVertexDst[i].pos.y	= (static_cast<float>(pVertexSrc[i].mPos[1]) * (kPosRangeMax - kPosRangeMin)) / static_cast<float>(0xFFFF) + kPosRangeMin;
		pVertexDst[i].uv.x	= (static_cast<float>(pVertexSrc[i].mUV[0]) * (kUVRangeMax - kUVRangeMin)) / static_cast<float>(0xFFFF) + kUVRangeMin;
		pVertexDst[i].uv.y	= (static_cast<float>(pVertexSrc[i].mUV[1]) * (kUVRangeMax - kUVRangeMin)) / static_cast<float>(0xFFFF) + kUVRangeMin;
		pVertexDst[i].col	= pVertexSrc[i].mColor;
	}

	// Draw command conversion
	unsigned int IndiceOffset	= 0;
	pCmdList->Flags				= ImDrawListFlags_AllowVtxOffset|ImDrawListFlags_AntiAliasedLines|ImDrawListFlags_AntiAliasedFill|ImDrawListFlags_AntiAliasedLinesUseTex;
	pCmdList->CmdBuffer.resize(pCmdDrawFrame->mDrawCount);
	pCmdList->IdxBuffer.resize(pDrawData->TotalIdxCount);
	
	// Following commands are all remote client rendering commands
	for(uint32_t drawIdx(0); drawIdx < pCmdDrawFrame->mDrawCount; ++drawIdx)
	{
		const NetImgui::Internal::ImguiDraw& cmdDrawSrc	= pCmdDrawFrame->mpDraws[drawIdx];
		ImDrawCmd& cmdDrawDst							= pCmdList->CmdBuffer[drawIdx];

		cmdDrawDst.ClipRect.x		= cmdDrawSrc.mClipRect[0];
		cmdDrawDst.ClipRect.y		= cmdDrawSrc.mClipRect[1];
		cmdDrawDst.ClipRect.z		= cmdDrawSrc.mClipRect[2];
		cmdDrawDst.ClipRect.w		= cmdDrawSrc.mClipRect[3];
		cmdDrawDst.VtxOffset		= cmdDrawSrc.mVtxOffset;
		cmdDrawDst.IdxOffset		= IndiceOffset;
		cmdDrawDst.ElemCount		= cmdDrawSrc.mIdxCount;
		cmdDrawDst.UserCallback		= nullptr;
		cmdDrawDst.UserCallbackData	= nullptr;		
		
		// Copy/Convert indices for this draw
		if (cmdDrawSrc.mIndexSize == 4)
		{
			memcpy(&pCmdList->IdxBuffer[IndiceOffset], &pCmdDrawFrame->mpIndices[cmdDrawSrc.mIdxOffset], cmdDrawSrc.mIdxCount*sizeof(uint32_t));
		}
		else
		{
			const uint16_t* pIndices = reinterpret_cast<const uint16_t*>(&pCmdDrawFrame->mpIndices[cmdDrawSrc.mIdxOffset]);
			for (uint32_t i(0); i < cmdDrawSrc.mIdxCount; ++i){
				pCmdList->IdxBuffer[IndiceOffset+i] = static_cast<ImDrawIdx>(pIndices[i]);
			}
		}		
		IndiceOffset += cmdDrawSrc.mIdxCount;
		
		// Find and assign wanted texture
		cmdDrawDst.TextureId = gEmptyTexture.mpHAL_Texture; // Default to empty texture
		for(size_t i=0; i<client.mvTextures.size(); ++i)
		{
			if( client.mvTextures[i].mImguiId == cmdDrawSrc.mTextureId )
			{
				cmdDrawDst.TextureId = client.mvTextures[i].mpHAL_Texture;
				break;
			}
		}
	}
	return pDrawData;
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
	strcpy_s(cmdlineClient.mClientName, "Commandline");

	while( *zEntryCur != 0 )
	{
		zEntryCur++;
		if( (*zEntryCur == ';' || *zEntryCur == 0) )
		{
			if (paramIndex == 0)
				strncpy_s(cmdlineClient.mHostName, zEntryStart, zEntryCur-zEntryStart);
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
		}

		NetImgui::Internal::ScopedImguiContext scopedCtx(client.mpBGContext);
		ImGui::GetIO().DisplaySize = ImVec2(client.mAreaSizeX,client.mAreaSizeY);
		ImGui::NewFrame();
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(client.mAreaSizeX,client.mAreaSizeY));
		ImGui::Begin("Background", nullptr, ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoSavedSettings);
		// Look for the desired texture (and use default if not found)
		const ServerTexture* pTexture = &UI::GetBackgroundTexture();
		for(size_t i=0; i<client.mvTextures.size(); ++i)
		{
			if( client.mvTextures[i].mImguiId == client.mBGSettings.mTextureId )
			{
				pTexture = &client.mvTextures[i];
				break;
			}
		}		
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
		if( client.mbIsConnected && client.mbIsVisible )
		{
			// Update the RenderTarget destination of each client, of size was updated
			if (client.mAreaSizeX > 0 && client.mAreaSizeY > 0 && (client.mAreaRTSizeX != client.mAreaSizeX || client.mAreaRTSizeY != client.mAreaSizeY))
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
			ImDrawData* pDrawData = CreateDrawData(client);
			if( pDrawData )
			{
				DrawClientBackground(client);
				HAL_RenderDrawData(client, pDrawData);
				DestroyDrawData(pDrawData);
			}
		}
	}
}

}} // namespace NetImguiServer { namespace App
