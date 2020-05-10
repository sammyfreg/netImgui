#include "stdafx.h"
#include "RemoteClient.h"
#include <Private/NetImGui_CmdPackets.h>

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
	//SF texture vector not threadsafe, fix this (used in com and main thread)
	size_t foundIdx((size_t)-1);
	for(size_t i=0; foundIdx == -1 && i<mvTextures.size(); i++)
	{
		if( mvTextures[i].mImguiId == pTextureCmd->mTextureId )
		{
			foundIdx = i;
			dx::TextureRelease( mvTextures[foundIdx] );			
		}
	}

	if( foundIdx == -1)
	{
		foundIdx = mvTextures.size();
		mvTextures.resize(foundIdx+1);
	}	

	mvTextures[foundIdx] = dx::TextureCreate(pTextureCmd);
}

void ClientRemote::Reset()
{
	for(const auto& texHandle : mvTextures )
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
	pInputNew->ScreenSize[0]	= static_cast<uint16_t>(rect.right - rect.left);
	pInputNew->ScreenSize[1]	= static_cast<uint16_t>(rect.bottom - rect.top);
	pInputNew->MousePos[0]		= static_cast<int16_t>(MousPos.x);
	pInputNew->MousePos[1]		= static_cast<int16_t>(MousPos.y);
	pInputNew->MouseWheelVert	= Input.mMouseWheelVertPos;
	pInputNew->MouseWheelHoriz	= Input.mMouseWheelHorizPos;

	//SF TODO Add Clipboard support

	// Avoid reading keyboard/mouse input if we are not the active window	
	uint8_t KeyStates[256];
	memset(pInputNew->KeysDownMask, 0, sizeof(pInputNew->KeysDownMask));	
	if( hWindows == GetFocus() && GetKeyboardState( KeyStates ) )
	{
		uint64_t Value(0);
		for(uint64_t i(0); i<ARRAY_COUNT(KeyStates); ++i)
		{
			Value |= (KeyStates[i] & 0x80) !=0 ? uint64_t(1) << (i % 64) : 0;
			if( (i+1) % 64 == 0 )
			{
				pInputNew->KeysDownMask[i/64] = Value;
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
		size_t keyCount(ARRAY_COUNT(pCmdInput->KeyChars));		
		mPendingKeys.ReadData(pCmdInput->KeyChars, keyCount);
		pCmdInput->KeyCharCount	= static_cast<uint8_t>(keyCount);
	}
	return pCmdInput;
}

