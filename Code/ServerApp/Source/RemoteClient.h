#pragma once

#include <vector>
#include <Private/NetImgui_CmdPackets.h>
#include "../DirectX/DirectX11.h"

struct InputUpdate 
{ 	
	float		mMouseWheelVertPos;
	float		mMouseWheelHorizPos;
	uint16_t	mKeys[128];
	uint8_t		mKeyCount;
	uint8_t		PADDING[3];
};

struct ClientRemote
{
	using ExchPtrFrame	= NetImgui::Internal::ExchangePtr<NetImgui::Internal::CmdDrawFrame>;
	using ExchPtrInput	= NetImgui::Internal::ExchangePtr<NetImgui::Internal::CmdInput>;
	using PendingKeys	= NetImgui::Internal::Ringbuffer<uint16_t, 1024>;
										ClientRemote();
										~ClientRemote();
										ClientRemote(const ClientRemote&)	= delete;
										ClientRemote(const ClientRemote&&)	= delete;
	void								operator=(const ClientRemote&) = delete;

	void								Reset();

	void								ReceiveTexture(NetImgui::Internal::CmdTexture*);	
	void								ReceiveDrawFrame(NetImgui::Internal::CmdDrawFrame*);
	NetImgui::Internal::CmdDrawFrame*	GetDrawFrame();
	
	void								UpdateInputToSend(HWND hWindows, InputUpdate& Input);
	NetImgui::Internal::CmdInput*		CreateInputCommand();

	char								mName[32]		= {0};
	unsigned char						mConnectIP[4]	= {0};
	unsigned int						mConnectPort	= static_cast<unsigned int>(-1);	
	UINT_PTR							mMenuId			= 0;		
	NetImgui::Internal::CmdDrawFrame*	mpFrameDraw		= nullptr;	// Current valid DrawFrame
	std::vector<dx::TextureHandle>		mvTextures;					// List of textures received and used by the client	
	ExchPtrFrame						mPendingFrame;				// Frame received and waiting to be displayed
	ExchPtrInput						mPendingInput;				// Input command waiting to be sent out to client
	PendingKeys							mPendingKeys;				// Character input waiting to be sent out to client
	bool								mbIsActive		= false;	// If currently selected client for display
	std::atomic_bool					mbConnected;				// If connected to a remote client
	uint8_t								PADDING[6];
};
