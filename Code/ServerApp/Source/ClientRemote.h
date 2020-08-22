#pragma once

#include <vector>
#include <chrono>
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
	static constexpr uint32_t kInvalidClient = static_cast<uint32_t>(-1);
	using ExchPtrFrame	= NetImgui::Internal::ExchangePtr<NetImgui::Internal::CmdDrawFrame>;
	using ExchPtrInput	= NetImgui::Internal::ExchangePtr<NetImgui::Internal::CmdInput>;
	using PendingKeys	= NetImgui::Internal::Ringbuffer<uint16_t, 1024>;
											ClientRemote();
											~ClientRemote();
											ClientRemote(const ClientRemote&)	= delete;
											ClientRemote(const ClientRemote&&)	= delete;
	void									operator=(const ClientRemote&) = delete;

	void									Reset();

	void									ReceiveTexture(NetImgui::Internal::CmdTexture*);	
	void									ReceiveDrawFrame(NetImgui::Internal::CmdDrawFrame*);
	NetImgui::Internal::CmdDrawFrame*		GetDrawFrame();
	
	void									UpdateInputToSend(HWND hWindows, InputUpdate& Input);
	NetImgui::Internal::CmdInput*			CreateInputCommand();

	char									mInfoName[128]			= {0};
	char									mInfoImguiVerName[16]	= {0};
	char									mInfoNetImguiVerName[16]= {0};
	uint32_t								mInfoImguiVerID			= 0;
	uint32_t								mInfoNetImguiVerID		= 0;
	char									mConnectHost[64]		= {0};	//!< Connected Hostname of this remote client
	unsigned int							mConnectPort;					//!< Connected Port of this remote client
	UINT_PTR								mMenuId;						//!< Application assigned 'MenuId' of menubar entry
	NetImgui::Internal::CmdDrawFrame*		mpFrameDraw;					//!< Current valid DrawFrame
	std::vector<dx::TextureHandle>			mvTextures;						//!< List of textures received and used by the client	
	ExchPtrFrame							mPendingFrame;					//!< Frame received and waiting to be displayed
	ExchPtrInput							mPendingInput;					//!< Input command waiting to be sent out to client
	PendingKeys								mPendingKeys;					//!< Character input waiting to be sent out to client
	bool									mbIsActive;						//!< If currently selected client for display
	std::atomic_bool						mbIsFree;						//!< If available to use for a new connected client
	std::atomic_bool						mbIsConnected;					//!< If connected to a remote client
	std::atomic_bool						mbPendingDisconnect;			//!< Server requested a disconnect on this item
	std::chrono::steady_clock::time_point	mConnectedTime;					//!< When the connection was established with this remote client
	uint32_t								mClientConfigID;				//!< ID of ClientConfig that connected (if connection came from our list of ClientConfigs)
	uint8_t									PADDING[4];

	static bool								Startup(uint32_t clientCountMax);
	static void								Shutdown();
	static uint32_t							GetCountMax();
	static uint32_t							GetFreeIndex();
	static ClientRemote&					Get(uint32_t index);	
};
