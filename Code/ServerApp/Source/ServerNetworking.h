#pragma once

#include <stdint.h>
struct ClientRemote;

namespace NetworkServer
{

bool Startup();
void Shutdown();
bool IsWaitingForConnection();

}
