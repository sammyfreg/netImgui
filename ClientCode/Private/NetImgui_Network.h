#pragma once

#include <stdint.h>

namespace NetImgui { namespace Internal { namespace Network 
{

struct SocketInfo;

bool		Startup		(void);
SocketInfo* Connect		(const uint8_t ServerIp[4], uint32_t ServerPort);
bool		DataReceive	(SocketInfo* pClientSocket, void* pDataIn, size_t Size);
bool		DataSend	(SocketInfo* pClientSocket, void* pDataOut, size_t Size);
void		Disconnect	(SocketInfo* pClientSocket);
void		Shutdown	(void);

}}} //namespace NetImgui::Internal::Network
