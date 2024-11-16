#pragma once

namespace NetImgui { namespace Internal { namespace Network 
{

struct SocketInfo;

bool		Startup				(void);
void		Shutdown			(void);

SocketInfo* Connect				(const char* ServerHost, uint32_t ServerPort);	// Communication Socket expected to be blocking
SocketInfo* ListenConnect		(SocketInfo* ListenSocket);						// Communication Socket expected to be blocking
SocketInfo* ListenStart			(uint32_t ListenPort);							// Listening Socket expected to be non blocking
void		Disconnect			(SocketInfo* pClientSocket);

bool		DataReceivePending	(SocketInfo* pClientSocket);								// True if some new data if waiting to be processed from remote connection
bool		DataReceive			(SocketInfo* pClientSocket, void* pDataIn, size_t Size);	// Read X amount of bytes to remote connection. Will idle until size request is fullfilled.
bool		DataSend			(SocketInfo* pClientSocket, void* pDataOut, size_t Size);	// Send x amount of bytes to remote connection. Will idle until size request is fullfilled.

}}} //namespace NetImgui::Internal::Network
