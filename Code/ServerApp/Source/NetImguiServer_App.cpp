#include "NetImguiServer_App.h"
#include "NetImguiServer_Config.h"
#include "NetImguiServer_Network.h"
#include "NetImguiServer_UI.h"
#include "NetImguiServer_RemoteClient.h"
#include <algorithm>

namespace NetImguiServer { namespace App
{

constexpr uint32_t kClientCountMax		= 32;	//! @sammyfreg todo: support unlimited client count
static void* gpHAL_EmptyTexture			= nullptr;

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
		NetImguiServer::App::HAL_CreateTexture(8, 8, NetImgui::eTexFormat::kTexFmtA8, EmptyPixels, gpHAL_EmptyTexture);
	
		//-----------------------------------------------------------------------------------------
		// Using a different default font (provided with Dear ImGui)
		//-----------------------------------------------------------------------------------------
		if (ImGui::GetIO().Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 16.0f) == nullptr) {
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
	NetImguiServer::App::HAL_DestroyTexture(gpHAL_EmptyTexture);
	NetImguiServer::Config::Client::Clear();
	RemoteClient::Client::Shutdown();
	HAL_Shutdown();
}

void SetupRenderState(const ImDrawList*, const ImDrawCmd*)
{
	HAL_RenderStateSetup();
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
	pCmdList->CmdBuffer.resize(pCmdDrawFrame->mDrawCount+1);
	pCmdList->IdxBuffer.resize(pDrawData->TotalIdxCount);

	// 1st Command is callback to update GPU state with our own blending settings
	pCmdList->CmdBuffer[0].UserCallback = SetupRenderState; 

	// Following commands are all remote client rendering commands
	for(uint32_t drawIdx(0); drawIdx < pCmdDrawFrame->mDrawCount; ++drawIdx)
	{
		const NetImgui::Internal::ImguiDraw& cmdDrawSrc	= pCmdDrawFrame->mpDraws[drawIdx];
		ImDrawCmd& cmdDrawDst							= pCmdList->CmdBuffer[1+drawIdx];

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
			memcpy(&pCmdList->IdxBuffer[IndiceOffset], &pCmdDrawFrame->mpIndices[cmdDrawSrc.mIdxOffset], cmdDrawSrc.mIdxCount);
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
		cmdDrawDst.TextureId = gpHAL_EmptyTexture; // Default to empty texture
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
	cmdlineClient.mTransient		= transient;
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


void UpdateRemoteContent()
{
	for (uint32_t i(0); i < RemoteClient::Client::GetCountMax(); ++i)
	{
		RemoteClient::Client& client = RemoteClient::Client::Get(i);
		if( client.mbIsConnected && client.mbIsVisible )
		{
			// Update the RenderTarget destination of each client, of size was updated
			if (client.mAreaRTSizeX != client.mAreaSizeX || client.mAreaRTSizeY != client.mAreaSizeY)
			{
				if (HAL_CreateRenderTarget(client.mAreaSizeX, client.mAreaSizeY, client.mpHAL_AreaRT, client.mpHAL_AreaTexture))
				{
					client.mAreaRTSizeX		= client.mAreaSizeX;
					client.mAreaRTSizeY		= client.mAreaSizeY;
					client.mLastUpdateTime	= std::chrono::steady_clock::now() - std::chrono::hours(1); // Will redraw the client
				}
			}

			// Render the remote results
			ImDrawData* pDrawData = CreateDrawData(client);
			if( pDrawData )
			{				
				//HAL_RenderRemoteClient();
				const float ClearColor[4] = {0.0f, 0.0f, 0.0f, 0.f};
				HAL_RenderDrawData(client.mpHAL_AreaRT, pDrawData, ClearColor);				
				DestroyDrawData(pDrawData);
			}
		}
	}
}



}} // namespace NetImguiServer { namespace App
