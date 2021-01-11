#include "NetImguiServer_App.h"
#include "NetImguiServer_RemoteClient.h"
#include "NetImguiServer_Config.h"
#include <Private/NetImgui_CmdPackets.h>

namespace NetImguiServer { namespace RemoteClient
{

static Client* gpClients	= nullptr;	// Table of all potentially connected clients to this server
static uint32_t gClientCountMax = 0;

Client::Client()
: mConnectPort(0)
, mpFrameDraw(nullptr)
, mbIsVisible(false)
, mbIsFree(true)
, mbIsConnected(false)
, mbPendingDisconnect(false)
, mClientConfigID(NetImguiServer::Config::Client::kInvalidRuntimeID)
, mClientIndex(0)
{
}

Client::~Client()
{
	Reset();
}

void Client::ReceiveDrawFrame(NetImgui::Internal::CmdDrawFrame* pFrameData)
{
	constexpr float hysteresisFactor	= 0.05f; // Between 0 to 1.0
	auto elapsedTime					= std::chrono::steady_clock::now() - mLastDrawFrame;
	float tmMs							= static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count());
	float newFPS						= 1000.f / tmMs;
	mStatsFPS							= mStatsFPS * (1.f-hysteresisFactor) + newFPS*hysteresisFactor;
	mLastDrawFrame						= std::chrono::steady_clock::now();
	mPendingFrame.Assign(pFrameData);	
}

void Client::ReceiveTexture(NetImgui::Internal::CmdTexture* pTextureCmd)
{
	if( pTextureCmd )
	{
		size_t foundIdx		= static_cast<size_t>(-1);
		bool isRemoval		= pTextureCmd->mFormat == NetImgui::eTexFormat::kTexFmt_Invalid;
		for(size_t i=0; foundIdx == static_cast<size_t>(-1) && i<mvTextures.size(); i++)
		{
			if( mvTextures[i].mImguiId == pTextureCmd->mTextureId )
			{
				foundIdx = i;
				NetImguiServer::App::HAL_DestroyTexture(mvTextures[foundIdx].mpHAL_Texture);
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
				mvTextures[foundIdx].mImguiId = pTextureCmd->mTextureId;
			}
			NetImguiServer::App::HAL_CreateTexture(pTextureCmd->mWidth, pTextureCmd->mHeight, pTextureCmd->mFormat, pTextureCmd->mpTextureData.Get(), mvTextures[foundIdx].mpHAL_Texture);
		}
	}
}

void Client::Reset()
{
	for(auto& texEntry : mvTextures )
	{
		NetImguiServer::App::HAL_DestroyTexture(texEntry.mpHAL_Texture);
	}
	mvTextures.clear();

	mPendingFrame.Free();
	mPendingInput.Free();
	netImguiDeleteSafe(mpFrameDraw);

	mInfoName[0]		= 0;
	mClientConfigID		= NetImguiServer::Config::Client::kInvalidRuntimeID;
	mClientIndex		= 0;
	mbPendingDisconnect	= false;
	mbIsConnected		= false;
	mbIsFree			= true;	

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
//
//=================================================================================================
NetImgui::Internal::CmdDrawFrame* Client::TakeDrawFrame()
{
	// Check if a new frame has been added. If yes, then take ownership of it.
	NetImgui::Internal::CmdDrawFrame* pPendingFrame = mPendingFrame.Release();
	if( pPendingFrame )
	{
		netImguiDeleteSafe( mpFrameDraw );
		mpFrameDraw = pPendingFrame;
	}	
	return mpFrameDraw;
}

//=================================================================================================
// Note: Caller must take ownership of item and delete the object
//=================================================================================================
NetImgui::Internal::CmdInput* Client::TakePendingInput()
{
	return mPendingInput.Release();
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
	// Try to re-acquire unsent input command, or create a new one if none pending
	NetImguiServer::Config::Client configClient;
	bool wasActive		= mbIsActive;
	mbIsActive			= ImGui::IsWindowFocused();
	float refreshFPS	= mbIsActive ? NetImguiServer::Config::Server::sRefreshFPSActive : NetImguiServer::Config::Server::sRefreshFPSInactive;	
	float elapsedMs		= static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - mLastUpdateTime).count());
	bool bRefresh		=	wasActive != mbIsActive ||	// Important to sent input to client losing focus, making sure it receives knows to release the keypress
							(elapsedMs > 60000.f)	||	// Keep 1 refresh per minute minimum
							(refreshFPS >= 0.01f && elapsedMs >= 1000.f/refreshFPS);								
	if( !bRefresh )
	{ 
		return;
	}
	
	NetImgui::Internal::CmdInput* pNewInput = TakePendingInput();
	pNewInput								= pNewInput ? pNewInput : NetImgui::Internal::netImguiNew<NetImgui::Internal::CmdInput>();

	// Update persistent mouse status
	const ImGuiIO& io = ImGui::GetIO();
	if( ImGui::IsMousePosValid(&io.MousePos)){
		mMousePos[0]				= io.MousePos.x - ImGui::GetCursorScreenPos().x;
		mMousePos[1]				= io.MousePos.y - ImGui::GetCursorScreenPos().y;		
	}

	// Create new Input command to send to client
	pNewInput->mScreenSize[0]	= static_cast<uint16_t>(ImGui::GetContentRegionAvail().x);
	pNewInput->mScreenSize[1]	= static_cast<uint16_t>(ImGui::GetContentRegionAvail().y);
	pNewInput->mMousePos[0]		= static_cast<int16_t>(mMousePos[0]);
	pNewInput->mMousePos[1]		= static_cast<int16_t>(mMousePos[1]);

	// Only capture keypress, mosue button, ... when window has the focus
	if( ImGui::IsWindowFocused() )
	{
		mMouseWheelPos[0] += io.MouseWheel;
		mMouseWheelPos[1] += io.MouseWheelH;
		
		pNewInput->mMouseWheelVert	= mMouseWheelPos[0];
		pNewInput->mMouseWheelHoriz	= mMouseWheelPos[1];
	
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

		size_t addedKeyCount		= std::min<size_t>(NetImgui::Internal::ArrayCount(pNewInput->mKeyChars)-pNewInput->mKeyCharCount, io.InputQueueCharacters.size());
		memcpy(&pNewInput->mKeyChars[pNewInput->mKeyCharCount], io.InputQueueCharacters.Data, addedKeyCount*sizeof(ImWchar));
		pNewInput->mKeyCharCount	+= static_cast<uint16_t>(addedKeyCount);
	}
	mPendingInput.Assign(pNewInput);
	mLastUpdateTime = std::chrono::steady_clock::now();
}

} } //namespace NetImguiServer { namespace Client