#pragma once

#include "RemoteImgui_Shared.h"
#include "RemoteImgui_CmdPackets.h"

struct ImDrawData;	//Forward declare of a dear imgui struct
struct ImGuiIO;		//Forward declare of a dear imgui struct

namespace RmtImgui { namespace Client
{

bool		Connect(const char* ClientName, const uint8_t ServerIp[4], uint32_t ServerPort);
void		Disconnect();
bool		IsConnected();

void		SendDataFrame(const ImDrawData* pImguiDrawData);
void		SendDataTexture(uint64_t textureId, void* pData, uint16_t width, uint16_t height, eTexFormat format);
bool		ReceiveDataInput(ImGuiIO& IoOut);

} } //namespace RmtImgui::Client
