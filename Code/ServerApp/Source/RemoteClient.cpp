#include "stdafx.h"
#include "RemoteClient.h"
#include <Private/NetImgui_CmdPackets.h>


ClientRemote::ClientRemote()
: mbConnected(false)
{
}

ClientRemote::~ClientRemote()
{
	Reset();
}

void ClientRemote::ReceiveDrawFrame(NetImgui::Internal::CmdDrawFrame* pFrameData)
{
	mPendingFrame.Assign(pFrameData);	
}

NetImgui::Internal::CmdDrawFrame* ClientRemote::GetDrawFrame()
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

void ClientRemote::ReceiveTexture(NetImgui::Internal::CmdTexture* pTextureCmd)
{
	//SF TODO texture vector not threadsafe, fix this (used in com and main thread)
	size_t foundIdx	= static_cast<size_t>(-1);
	bool isRemoval	= pTextureCmd->mFormat == NetImgui::kTexFmt_Invalid;
	for(size_t i=0; foundIdx == static_cast<size_t>(-1) && i<mvTextures.size(); i++)
	{
		if( mvTextures[i].mImguiId == pTextureCmd->mTextureId )
		{
			foundIdx = i;
			dx::TextureRelease( mvTextures[foundIdx] );			
		}
	}

	if( !isRemoval )
	{
		if( foundIdx == static_cast<size_t>(-1))
		{
			foundIdx = mvTextures.size();
			mvTextures.resize(foundIdx+1);
		}	
		mvTextures[foundIdx] = dx::TextureCreate(pTextureCmd);
	}
}

void ClientRemote::Reset()
{
	for(auto& texHandle : mvTextures )
		dx::TextureRelease(texHandle);
	mvTextures.clear();

	mMenuId		= 0;
	mName[0]	= 0;
	mPendingFrame.Free();
	mPendingInput.Free();
	netImguiDeleteSafe(mpFrameDraw);
}

void ClientRemote::UpdateInputToSend(HWND hWindows, InputUpdate& Input) 
{
	RECT rect;
	POINT MousPos;
	
	GetClientRect(hWindows, &rect);
	GetCursorPos(&MousPos);
	ScreenToClient(hWindows, &MousPos);

	auto* pInputNew				= NetImgui::Internal::netImguiNew<NetImgui::Internal::CmdInput>();
	pInputNew->mScreenSize[0]	= static_cast<uint16_t>(rect.right - rect.left);
	pInputNew->mScreenSize[1]	= static_cast<uint16_t>(rect.bottom - rect.top);
	pInputNew->mMousePos[0]		= static_cast<int16_t>(MousPos.x);
	pInputNew->mMousePos[1]		= static_cast<int16_t>(MousPos.y);
	pInputNew->mMouseWheelVert	= Input.mMouseWheelVertPos;
	pInputNew->mMouseWheelHoriz	= Input.mMouseWheelHorizPos;

	//SF TODO Add Clipboard support

	// Avoid reading keyboard/mouse input if we are not the active window	
	uint8_t KeyStates[256];
	memset(pInputNew->mKeysDownMask, 0, sizeof(pInputNew->mKeysDownMask));	
	if( hWindows == GetFocus() && GetKeyboardState( KeyStates ) )
	{
		uint64_t Value(0);
		for(uint64_t i(0); i<NetImgui::Internal::ArrayCount(KeyStates); ++i)
		{
			Value |= (KeyStates[i] & 0x80) !=0 ? uint64_t(1) << (i % 64) : 0;
			if( (i+1) % 64 == 0 )
			{
				pInputNew->mKeysDownMask[i/64] = Value;
				Value = 0;
			}
		}
	}
	
	size_t keyCount(Input.mKeyCount);
	mPendingKeys.AddData(Input.mKeys, keyCount);

	Input.mKeyCount = 0;
	mPendingInput.Assign(pInputNew);
}

NetImgui::Internal::CmdInput* ClientRemote::CreateInputCommand()
{
	NetImgui::Internal::CmdInput* pCmdInput = mPendingInput.Release();
	// Copy all of the character input into current command
	if( pCmdInput )
	{		
		size_t keyCount(NetImgui::Internal::ArrayCount(pCmdInput->mKeyChars));		
		mPendingKeys.ReadData(pCmdInput->mKeyChars, keyCount);
		pCmdInput->mKeyCharCount	= static_cast<uint8_t>(keyCount);
	}
	return pCmdInput;
}

