// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Few warnings we want disabled for the Server code
#include "../targetver.h"
#define OEMRESOURCE
#define WIN32_LEAN_AND_MEAN			// Exclude rarely-used stuff from Windows headers
#include <Windows.h>				// Windows Header Files

#include "WarningDisable.h"

#define SafeDelete(pData) if(pData){ delete pData; pData = nullptr; }
#define SafeDeleteArray(pData) if(pData){ delete[] pData; pData = nullptr; }