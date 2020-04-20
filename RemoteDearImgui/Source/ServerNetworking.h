#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <RemoteImgui_ImguiFrame.h>
#include <RemoteImgui_CmdPackets.h>
#include "../DirectX/DirectX11.h"

namespace NetworkServer
{

struct ClientInfo
{
												~ClientInfo();
	void										Reset();
	void										AddFrame(RmtImgui::ImguiFrame*);
	RmtImgui::ImguiFrame*						GetFrame();
	void										SetTexture(const RmtImgui::CmdTexture* pTextureCmd);
	void										UpdateInput(HWND hWindows, RmtImgui::ImguiInput& Input);
	std::atomic_bool							mbConnected		= false;
	unsigned char								mConnectIP[4];
	unsigned int								mConnectPort	= (unsigned int)-1;	
	char										mName[32];
	UINT_PTR									mMenuId			= 0;
	bool										mbIsActive		= false;
	RmtImgui::ImguiFrame*						mpFrameDraw		= nullptr;
	std::vector<dx::TextureHandle>				mvTextures;
	RmtImgui::ExchangePtr<RmtImgui::ImguiFrame>	mPendingFrame;
	RmtImgui::ExchangePtr<RmtImgui::ImguiInput>	mPendingInput;	
};

bool Startup(ClientInfo* pClients, uint32_t ClientCount, uint32_t ListenPort);
void Shutdown();

}