#pragma once

#include <stdint.h>
struct ClientRemote;

namespace NetworkServer
{

bool Startup(uint32_t ListenPort);
void Shutdown();

}
