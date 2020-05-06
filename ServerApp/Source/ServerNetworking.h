#pragma once

#include <stdint.h>
struct ClientRemote;

namespace NetworkServer
{

bool Startup(ClientRemote* pClients, uint32_t ClientCount, uint32_t ListenPort);
void Shutdown();

}