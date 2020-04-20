#pragma once

#include <stdint.h>

namespace RmtImgui { namespace Network
{

constexpr uint32_t kDefaultServerPort = 8888;

struct SocketInfo;

bool		Startup();
SocketInfo* Connect(const uint8_t ServerIp[4], uint32_t ServerPort);
bool		DataReceive(SocketInfo* pClientSocket, void* pDataIn, size_t Size);
bool		DataSend(SocketInfo* pClientSocket, void* pDataOut, size_t Size);
void		Disconnect(SocketInfo* pClientSocket);
void		Shutdown();

} } //namespace RmtImgui::Network