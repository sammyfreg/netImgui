// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Few warnings we want disabled for the Server code
#if defined(__clang__)
	#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
	#pragma clang diagnostic ignored "-Wmissing-prototypes"

#elif defined(_MSC_VER) 
	#pragma warning (disable: 4464)	// warning C4464: relative include path contains '..'
	#pragma warning (disable: 4365)	// warning C4365 : 'xxx' : conversion from 'yyy' to 'unsigned zzz', signed / unsigned mismatch
	#pragma warning (disable: 4774)	// warning C4774 : xxx : format string expected in argument yyy is not a string literal
	#pragma warning (disable: 5045)	// warning C5045 : Compiler will insert Spectre mitigation for memory load if / Qspectre switch specified
	#pragma warning (disable: 4710)	// warning C4710: 'xxx': function not inlined
	#pragma warning (disable: 4711) // warning C4711: function 'xxx' selected for automatic inline expansion
#endif

#include "../targetver.h"
#define OEMRESOURCE
#define WIN32_LEAN_AND_MEAN			// Exclude rarely-used stuff from Windows headers
#include <Windows.h>				// Windows Header Files
