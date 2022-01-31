#include "NetImguiServer_App.h"
#include "NetImguiServer_RemoteClient.h"
#include "NetImguiServer_Config.h"
#include "NetImguiServer_UI.h"
#include <Private/NetImgui_CmdPackets.h>
#include <algorithm>

namespace NetImguiServer { namespace RemoteClient
{

static Client* gpClients		= nullptr;	// Table of all potentially connected clients to this server
static uint32_t gClientCountMax = 0;

NetImguiImDrawData::NetImguiImDrawData()
: mCommandList(nullptr)
{
	CmdListsCount	= 1;	// All draws collapsed in same CmdList
	mpCommandList	= &mCommandList;
	CmdLists		= &mpCommandList;
}


Client::Client()
: mPendingTextureReadIndex(0)
, mPendingTextureWriteIndex(0)
, mbIsVisible(false)
, mbIsFree(true)
, mbIsConnected(false)
, mbDisconnectPending(false)
, mbCompressionSkipOncePending(false)
, mClientConfigID(NetImguiServer::Config::Client::kInvalidRuntimeID)
{
}

Client::~Client()
{
	Reset();
	
	//Note: Reset is usually called from com thread, can't destroy ImGui Context in it, 
	//		or drawing data that could be currently in use by server to draw client results
	NetImgui::Internal::netImguiDeleteSafe(mpImguiDrawData);
	NetImgui::Internal::netImguiDeleteSafe(mpFrameDrawPrev);
	if (mpBGContext) {
		ImGui::DestroyContext(mpBGContext);
		mpBGContext	= nullptr;
	}
}

void Client::ReceiveDrawFrame(NetImgui::Internal::CmdDrawFrame* pFrameData)
{
	if( pFrameData->mCompressed )
	{
		if( mpFrameDrawPrev != nullptr && (mpFrameDrawPrev->mFrameIndex+1) == pFrameData->mFrameIndex ) {
			NetImgui::Internal::CmdDrawFrame* pUncompressedFrame = NetImgui::Internal::DecompressCmdDrawFrame(mpFrameDrawPrev, pFrameData);
			netImguiDeleteSafe( pFrameData );
			pFrameData = pUncompressedFrame;
		}
		// Missing previous frame data
		// ignore this drawframe and request a new uncompressed one to be able to resume display
		else
		{
			mbCompressionSkipOncePending = true;
			netImguiDeleteSafe( pFrameData );
		}
	}

	netImguiDeleteSafe( mpFrameDrawPrev );
	if( pFrameData )
	{
		// Convert DrawFrame command to Dear Imgui DrawData,
		// and make it available for main thread to use in rendering
		mpFrameDrawPrev						= pFrameData;
		NetImguiImDrawData*	pNewDrawData	= ConvertToImguiDrawData(pFrameData);
		mPendingImguiDrawDataIn.Assign(pNewDrawData);
		
		// Update framerate
		constexpr float kHysteresis	= 0.05f; // Between 0 to 1.0
		auto elapsedTime			= std::chrono::steady_clock::now() - mLastDrawFrame;
		float tmMicroS				= static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(elapsedTime).count());
		float newFPS				= 1000000.f / tmMicroS;
		mStatsFPS					= mStatsFPS * (1.f-kHysteresis) + newFPS*kHysteresis;
		mLastDrawFrame				= std::chrono::steady_clock::now();
	}
}

void Client::ReceiveTexture(NetImgui::Internal::CmdTexture* pTextureCmd)
{
	if( pTextureCmd )
	{
		// Wait for a free spot in the ring buffer
		while ( mPendingTextureWriteIndex-mPendingTextureReadIndex > IM_ARRAYSIZE(mpPendingTextures) );
		mpPendingTextures[(mPendingTextureWriteIndex++) % IM_ARRAYSIZE(mpPendingTextures)] = pTextureCmd;
	}
}

void Client::ProcessPendingTextures()
{
	while( mPendingTextureReadIndex < mPendingTextureWriteIndex )
	{
		NetImgui::Internal::CmdTexture* pTextureCmd = mpPendingTextures[(mPendingTextureReadIndex++) % IM_ARRAYSIZE(mpPendingTextures)];
		size_t foundIdx								= static_cast<size_t>(-1);
		bool isRemoval								= pTextureCmd->mFormat == NetImgui::eTexFormat::kTexFmt_Invalid;
		for(size_t i=0; foundIdx == static_cast<size_t>(-1) && i<mvTextures.size(); i++)
		{
			if( mvTextures[i].mImguiId == pTextureCmd->mTextureId )
			{
				foundIdx = i;
				NetImguiServer::App::HAL_DestroyTexture(mvTextures[foundIdx]);
				if( isRemoval )
				{
					mvTextures[foundIdx] = mvTextures.back();
					mvTextures.pop_back();
				}
			}
		}

		if( !isRemoval )
		{		
			if( foundIdx == static_cast<size_t>(-1))
			{
				foundIdx = mvTextures.size();
				mvTextures.resize(foundIdx+1);
			}
			
			NetImguiServer::App::HAL_CreateTexture(pTextureCmd->mWidth, pTextureCmd->mHeight, static_cast<NetImgui::eTexFormat>(pTextureCmd->mFormat), pTextureCmd->mpTextureData.Get(), mvTextures[foundIdx]);
			mvTextures[foundIdx].mImguiId = pTextureCmd->mTextureId;
		}

		NetImgui::Internal::netImguiDeleteSafe(pTextureCmd);
	}
}

void Client::Reset()
{
	for(auto& texEntry : mvTextures )
	{
		NetImguiServer::App::HAL_DestroyTexture(texEntry);
	}
	mvTextures.clear();

	mPendingImguiDrawDataIn.Free();
	mPendingBackgroundIn.Free();
	mPendingInputOut.Free();

	mInfoName[0]					= 0;
	mClientConfigID					= NetImguiServer::Config::Client::kInvalidRuntimeID;
	mClientIndex					= 0;
	mbCompressionSkipOncePending	= false;
	mbDisconnectPending				= false;
	mbIsConnected					= false;
	mbIsFree						= true;
	mBGNeedUpdate					= true;

}

void Client::Initialize()
{
	mConnectedTime		= std::chrono::steady_clock::now();
	mLastUpdateTime		= std::chrono::steady_clock::now() - std::chrono::hours(1);
	mLastDrawFrame		= std::chrono::steady_clock::now();
	mStatsIndex			= 0;
	mStatsRcvdBps		= 0;
	mStatsSentBps		= 0;
	mStatsFPS			= 0.f;
	mStatsDataRcvd		= 0;
	mStatsDataSent		= 0;
	mStatsDataRcvdPrev	= 0;
	mStatsDataSentPrev	= 0;
	mStatsTime			= std::chrono::steady_clock::now();
	mBGSettings			= NetImgui::Internal::CmdBackground();	// Assign background default value, until we receive first update from client
	NetImgui::Internal::netImguiDeleteSafe(mpImguiDrawData);
	NetImgui::Internal::netImguiDeleteSafe(mpFrameDrawPrev);
}

bool Client::Startup(uint32_t clientCountMax)
{
	gClientCountMax = clientCountMax;
	gpClients		= new Client[clientCountMax];
	return gpClients != nullptr;
}

void Client::Shutdown()
{
	gClientCountMax = 0;
	if (gpClients)
	{
		delete[] gpClients;
		gpClients = nullptr;
	}
}

uint32_t Client::GetCountMax()
{
	return gClientCountMax;
}

Client& Client::Get(uint32_t index)
{
	bool bValid = gpClients && index < gClientCountMax;
	static Client sInvalidClient;
	assert( bValid );
	return bValid ? gpClients[index] : sInvalidClient;
}

uint32_t Client::GetFreeIndex()
{
	for (uint32_t i(0); i < gClientCountMax; ++i)
	{
		if( gpClients[i].mbIsFree.exchange(false) == true )
			return i;
	}
	return kInvalidClient;
}

//=================================================================================================
// Get the current Dear Imgui drawdata to use for this client rendering content
//=================================================================================================
NetImguiImDrawData*	Client::GetImguiDrawData(void* pEmtpyTextureHAL)
{
	// Check if a new frame has been added. If yes, then take ownership of it.
	NetImguiImDrawData* pPendingDrawData = mPendingImguiDrawDataIn.Release();
	if( pPendingDrawData )
	{
		NetImgui::Internal::netImguiDeleteSafe( mpImguiDrawData );
		mpImguiDrawData				= pPendingDrawData;
		const size_t clientTexCount = mvTextures.size();
		
		// When a new drawdata is available, need to convert the textureid from NetImgui Id
		// to the backend renderer format (texture view pointer). 
		// Done here (in main thread) instead of when first received on the (com thread),
		// since 'mvTextures' can only be safely accessed on (main thread).
		for(int i(0); i<pPendingDrawData->CmdListsCount; ++i)
		{
			ImDrawList* pCmdList = pPendingDrawData->CmdLists[i];
			for(int drawIdx(0), drawCount(pCmdList->CmdBuffer.size()); drawIdx<drawCount; ++drawIdx)
			{
				uint64_t wantedTexID					= NetImgui::Internal::TextureCastHelper(pCmdList->CmdBuffer[drawIdx].TextureId);
				pCmdList->CmdBuffer[drawIdx].TextureId	= pEmtpyTextureHAL; // Default to empty texture
				for(size_t texIdx=0; texIdx<clientTexCount; ++texIdx)
				{
					if( mvTextures[texIdx].mImguiId == wantedTexID )
					{
						pCmdList->CmdBuffer[drawIdx].TextureId	= mvTextures[texIdx].mpHAL_Texture;
						break;
					}
				}
			}
		}
	}	
	return mpImguiDrawData;
}

//=================================================================================================
// Create a new Dear Imgui DrawData ready to be submitted for rendering
//=================================================================================================
NetImguiImDrawData* Client::ConvertToImguiDrawData(const NetImgui::Internal::CmdDrawFrame* pCmdDrawFrame)
{
	constexpr float kPosRangeMin	= static_cast<float>(NetImgui::Internal::ImguiVert::kPosRange_Min);
	constexpr float kPosRangeMax	= static_cast<float>(NetImgui::Internal::ImguiVert::kPosRange_Max);
	constexpr float kUVRangeMin		= static_cast<float>(NetImgui::Internal::ImguiVert::kUvRange_Min);
	constexpr float kUVRangeMax		= static_cast<float>(NetImgui::Internal::ImguiVert::kUvRange_Max);

	if (!pCmdDrawFrame){
		return nullptr;
	}
	mMouseCursor					= static_cast<ImGuiMouseCursor>(pCmdDrawFrame->mMouseCursor);

	NetImguiImDrawData* pDrawData	= NetImgui::Internal::netImguiNew<NetImguiImDrawData>();
	ImDrawList* pCmdList			= pDrawData->CmdLists[0];
	pDrawData->Valid				= true;
    pDrawData->TotalVtxCount		= static_cast<int>(pCmdDrawFrame->mTotalVerticeCount);
	pDrawData->TotalIdxCount		= static_cast<int>(pCmdDrawFrame->mTotalIndiceCount);
    pDrawData->DisplayPos.x			= pCmdDrawFrame->mDisplayArea[0];
	pDrawData->DisplayPos.y			= pCmdDrawFrame->mDisplayArea[1];
    pDrawData->DisplaySize.x		= pCmdDrawFrame->mDisplayArea[2] - pCmdDrawFrame->mDisplayArea[0];
	pDrawData->DisplaySize.y		= pCmdDrawFrame->mDisplayArea[3] - pCmdDrawFrame->mDisplayArea[1];
    pDrawData->FramebufferScale		= ImVec2(1,1); //! @sammyfreg Currently untested, so force set to 1
    pDrawData->OwnerViewport		= nullptr;

	uint32_t indexOffset(0), vertexOffset(0);
	pCmdList->IdxBuffer.resize(pCmdDrawFrame->mTotalIndiceCount);
	pCmdList->VtxBuffer.resize(pCmdDrawFrame->mTotalVerticeCount);
	pCmdList->CmdBuffer.resize(pCmdDrawFrame->mTotalDrawCount);
	pCmdList->Flags					= ImDrawListFlags_AllowVtxOffset|ImDrawListFlags_AntiAliasedLines|ImDrawListFlags_AntiAliasedFill|ImDrawListFlags_AntiAliasedLinesUseTex;
	ImDrawIdx* pIndexDst			= &pCmdList->IdxBuffer[0];
	ImDrawVert* pVertexDst			= &pCmdList->VtxBuffer[0];
	ImDrawCmd* pCommandDst			= &pCmdList->CmdBuffer[0];

	for(uint32_t i(0); i<pCmdDrawFrame->mDrawGroupCount; ++i){
		const NetImgui::Internal::ImguiDrawGroup& drawGroup = pCmdDrawFrame->mpDrawGroups[i];
				
		// Copy/Convert Indices from network command to Dear ImGui indices format
		const uint16_t* pIndices = reinterpret_cast<const uint16_t*>(drawGroup.mpIndices.Get());
		if (drawGroup.mBytePerIndex == sizeof(ImDrawIdx))
		{
			memcpy(pIndexDst, pIndices, drawGroup.mIndiceCount*sizeof(ImDrawIdx));
		}
		else
		{
			for (uint32_t indexIdx(0); indexIdx < drawGroup.mIndiceCount; ++indexIdx){
				pIndexDst[indexIdx] = static_cast<ImDrawIdx>(pIndices[indexIdx]);
			}
		}

		// Convert the Vertices from network command to Dear Imgui Format
		const NetImgui::Internal::ImguiVert* pVertexSrc = drawGroup.mpVertices.Get();
		for (uint32_t vtxIdx(0); vtxIdx < drawGroup.mVerticeCount; ++vtxIdx)
		{
			pVertexDst[vtxIdx].pos.x				= (static_cast<float>(pVertexSrc[vtxIdx].mPos[0]) * (kPosRangeMax - kPosRangeMin)) / static_cast<float>(0xFFFF) + kPosRangeMin + drawGroup.mReferenceCoord[0];
			pVertexDst[vtxIdx].pos.y				= (static_cast<float>(pVertexSrc[vtxIdx].mPos[1]) * (kPosRangeMax - kPosRangeMin)) / static_cast<float>(0xFFFF) + kPosRangeMin + drawGroup.mReferenceCoord[1];
			pVertexDst[vtxIdx].uv.x					= (static_cast<float>(pVertexSrc[vtxIdx].mUV[0]) * (kUVRangeMax - kUVRangeMin)) / static_cast<float>(0xFFFF) + kUVRangeMin;
			pVertexDst[vtxIdx].uv.y					= (static_cast<float>(pVertexSrc[vtxIdx].mUV[1]) * (kUVRangeMax - kUVRangeMin)) / static_cast<float>(0xFFFF) + kUVRangeMin;
			pVertexDst[vtxIdx].col					= pVertexSrc[vtxIdx].mColor;
		}

		// Convert the Draws from network command to Dear Imgui Format
		const NetImgui::Internal::ImguiDraw* pDrawSrc = drawGroup.mpDraws.Get();
		for(uint32_t drawIdx(0); drawIdx<drawGroup.mDrawCount; ++drawIdx)
		{
			pCommandDst[drawIdx].ClipRect.x			= pDrawSrc[drawIdx].mClipRect[0];
			pCommandDst[drawIdx].ClipRect.y			= pDrawSrc[drawIdx].mClipRect[1];
			pCommandDst[drawIdx].ClipRect.z			= pDrawSrc[drawIdx].mClipRect[2];
			pCommandDst[drawIdx].ClipRect.w			= pDrawSrc[drawIdx].mClipRect[3];
			pCommandDst[drawIdx].VtxOffset			= pDrawSrc[drawIdx].mVtxOffset + vertexOffset;
			pCommandDst[drawIdx].IdxOffset			= pDrawSrc[drawIdx].mIdxOffset + indexOffset;
			pCommandDst[drawIdx].ElemCount			= pDrawSrc[drawIdx].mIdxCount;
			pCommandDst[drawIdx].UserCallback		= nullptr;
			pCommandDst[drawIdx].UserCallbackData	= nullptr;
			pCommandDst[drawIdx].TextureId			= NetImgui::Internal::TextureCastHelper(pDrawSrc[drawIdx].mTextureId);
		}
	
		pIndexDst		+= drawGroup.mIndiceCount;
		pVertexDst		+= drawGroup.mVerticeCount;
		pCommandDst		+= drawGroup.mDrawCount;
		indexOffset		+= drawGroup.mIndiceCount;
		vertexOffset	+= drawGroup.mVerticeCount;
	}
	return pDrawData;
}

//=================================================================================================
// Note: Caller must take ownership of item and delete the object
//=================================================================================================
NetImgui::Internal::CmdInput* Client::TakePendingInput()
{
	return mPendingInputOut.Release();
}

//=================================================================================================
// Capture current received Dear ImGui input, and forward it to the active client
// Note:	Even if a client is not focused, we are still sending it the mouse position, 
//			so it can update its UI.
// Note:	Sending an input command, will trigger a redraw on the client, 
//			which we receive on the server afterward
//=================================================================================================
void Client::CaptureImguiInput()
{
	// Capture input from Dear ImGui (when this Client is in focus)
	const ImGuiIO& io = ImGui::GetIO();
	if( ImGui::IsWindowFocused() ) {
		const size_t initialSize	= mPendingInputChars.size();
		const size_t addedChar		= io.InputQueueCharacters.size();
		if( addedChar ){
			mPendingInputChars.resize(initialSize+addedChar);
			memcpy(&mPendingInputChars[initialSize], io.InputQueueCharacters.Data, addedChar*sizeof(ImWchar));
		}

		mMouseWheelPos[0] += io.MouseWheel;
		mMouseWheelPos[1] += io.MouseWheelH;
	}

	// Update persistent mouse status	
	if( ImGui::IsMousePosValid(&io.MousePos)){
		mMousePos[0] = io.MousePos.x - ImGui::GetCursorScreenPos().x;
		mMousePos[1] = io.MousePos.y - ImGui::GetCursorScreenPos().y;		
	}

	// This method is tied to the Server VSync setting, which might not match our client desired refresh setting
	// When client refresh drops too much, take into consideration the lenght of the Server frame, to evaluate if we should update or not
	bool wasActive		= mbIsActive;
	mbIsActive			= ImGui::IsWindowFocused();
	float refreshFPS	= mbIsActive ? NetImguiServer::Config::Server::sRefreshFPSActive : NetImguiServer::Config::Server::sRefreshFPSInactive;	
	float elapsedMs		= static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - mLastUpdateTime).count()) / 1000.f;
	elapsedMs			+= mStatsFPS < refreshFPS ? 1000.f / (NetImguiServer::UI::GetDisplayFPS()/2.f) : 0.f;
	bool bRefresh		=	wasActive != mbIsActive ||						// Important to sent input to client losing focus, making sure it knows to release the keypress
							(elapsedMs > 60000.f)	||						// Keep 1 refresh per minute minimum
							(refreshFPS >= 0.01f && elapsedMs > 1000.f/refreshFPS);	

	if( !bRefresh ){
		return;
	}

	// Try to re-acquire unsent input command, or create a new one if none pending
	NetImgui::Internal::CmdInput* pNewInput = TakePendingInput();
	pNewInput								= pNewInput ? pNewInput : NetImgui::Internal::netImguiNew<NetImgui::Internal::CmdInput>();

	// Create new Input command to send to client
	pNewInput->mScreenSize[0]		= static_cast<uint16_t>(ImGui::GetContentRegionAvail().x);
	pNewInput->mScreenSize[1]		= static_cast<uint16_t>(ImGui::GetContentRegionAvail().y);
	pNewInput->mMousePos[0]			= static_cast<int16_t>(mMousePos[0]);
	pNewInput->mMousePos[1]			= static_cast<int16_t>(mMousePos[1]);
	pNewInput->mMouseWheelVert		= mMouseWheelPos[0];
	pNewInput->mMouseWheelHoriz		= mMouseWheelPos[1];
	pNewInput->mCompressionUse		= NetImguiServer::Config::Server::sCompressionEnable;
	pNewInput->mCompressionSkip		= mbCompressionSkipOncePending;
	mbCompressionSkipOncePending	= false;

	if( ImGui::IsWindowFocused() )
	{
		NetImguiServer::App::HAL_ConvertKeyDown(io.KeysDown, pNewInput->mKeysDownMask);
		pNewInput->SetKeyDown(NetImgui::Internal::CmdInput::eVirtualKeys::vkMouseBtnLeft,	io.MouseDown[0]);
		pNewInput->SetKeyDown(NetImgui::Internal::CmdInput::eVirtualKeys::vkMouseBtnRight,	io.MouseDown[1]);
		pNewInput->SetKeyDown(NetImgui::Internal::CmdInput::eVirtualKeys::vkMouseBtnMid,	io.MouseDown[2]);
		pNewInput->SetKeyDown(NetImgui::Internal::CmdInput::eVirtualKeys::vkMouseBtnExtra1, io.MouseDown[3]);
		pNewInput->SetKeyDown(NetImgui::Internal::CmdInput::eVirtualKeys::vkMouseBtnExtra2, io.MouseDown[4]);
		pNewInput->SetKeyDown(NetImgui::Internal::CmdInput::eVirtualKeys::vkKeyboardShift,	io.KeyShift);
		pNewInput->SetKeyDown(NetImgui::Internal::CmdInput::eVirtualKeys::vkKeyboardCtrl,	io.KeyCtrl);
		pNewInput->SetKeyDown(NetImgui::Internal::CmdInput::eVirtualKeys::vkKeyboardAlt,	io.KeyAlt);
		pNewInput->SetKeyDown(NetImgui::Internal::CmdInput::eVirtualKeys::vkKeyboardSuper1, io.KeySuper);

		//! @sammyfreg: ToDo Add support for gamepad
	}

	// Copy waiting characters inputs
	size_t addedKeyCount	= std::min<size_t>(NetImgui::Internal::ArrayCount(pNewInput->mKeyChars)-pNewInput->mKeyCharCount, mPendingInputChars.size());
	if( addedKeyCount ){
		memcpy(&pNewInput->mKeyChars[pNewInput->mKeyCharCount], &mPendingInputChars[0], addedKeyCount*sizeof(ImWchar));
		pNewInput->mKeyCharCount	+= static_cast<uint16_t>(addedKeyCount);
		size_t charRemainCount		= mPendingInputChars.size() - addedKeyCount;
		if( charRemainCount > 0 ){
			memcpy(&mPendingInputChars[0], &mPendingInputChars[addedKeyCount], mPendingInputChars.size()-addedKeyCount);
		}
		mPendingInputChars.resize( charRemainCount );
	}

	mPendingInputOut.Assign(pNewInput);
	mLastUpdateTime = std::chrono::steady_clock::now();
}

} } //namespace NetImguiServer { namespace Client