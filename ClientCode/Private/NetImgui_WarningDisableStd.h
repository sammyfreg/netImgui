
//
// Deactivate a few more warnings to allow standard header includes,  
// without generating warnings in '-Wall' compile actions enabled
//

#include "NetImgui_WarningDisable.h"

//=================================================================================================
// Clang
//=================================================================================================
#if defined (__clang__)


//=================================================================================================
// Visual Studio warnings
//=================================================================================================
#elif defined(_MSC_VER) 
	#pragma warning (disable: 4820)	// warning C4820 : xxx : yyy bytes padding added after data member zzz	
	#pragma warning (disable: 4668)	// warning C4668 : xxxx is not defined as a preprocessor macro, replacing with '0' for '#if/#elif' (winsock2.h)
	#pragma warning (disable: 4061) // warning C4061: enumerator xxx in switch of enum yyy is not explicitly handled by a case label (d3d11.h)
#endif

