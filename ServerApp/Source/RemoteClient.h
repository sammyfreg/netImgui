#pragma once

#include <vector>
#include <Private/NetImgui_CmdPackets.h>
#include "../DirectX/DirectX11.h"

struct InputUpdate 
{ 
	float		mMouseWheelVertPos;
	float		mMouseWheelHorizPos;
	uint8_t		mKeyCount;
	uint16_t	mKeys[128];
};

struct ClientRemote
{
	using ExchPtrFrame	= NetImgui::Internal::ExchangePtr<NetImgui::Internal::CmdDrawFrame>;
	using ExchPtrInput	= NetImgui::Internal::ExchangePtr<NetImgui::Internal::CmdInput>;
	using PendingKeys	= NetImgui::Internal::Ringbuffer<uint16_t, 1024>;
										ClientRemote();
										~ClientRemote();
	void								Reset();

	void								ReceiveTexture(NetImgui::Internal::CmdTexture*);	
	void								ReceiveDrawFrame(NetImgui::Internal::CmdDrawFrame*);
	NetImgui::Internal::CmdDrawFrame*	GetDrawFrame();
	
	void								UpdateInputToSend(HWND hWindows, InputUpdate& Input);
	NetImgui::Internal::CmdInput*		CreateInputCommand();

	char								mName[32];
	unsigned char						mConnectIP[4];
	unsigned int						mConnectPort	= static_cast<unsigned int>(-1);	
	UINT_PTR							mMenuId			= 0;
	std::atomic_bool					mbConnected;				// If connected to a remote client
	bool								mbIsActive		= false;	// If currently selected client for display
	NetImgui::Internal::CmdDrawFrame*	mpFrameDraw		= nullptr;	// Current valid DrawFrame
	std::vector<dx::TextureHandle>		mvTextures;					// List of textures received and used by the client
	ExchPtrFrame						mPendingFrame;				// Frame received and waiting to be displayed
	ExchPtrInput						mPendingInput;				// Input command waiting to be sent out to client
	PendingKeys							mPendingKeys;				// Character input waiting to be sent out to client
};
