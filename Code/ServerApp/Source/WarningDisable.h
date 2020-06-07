//
// Deactivate a few warnings to allow netImguiApp to compile 
// with 'Warning as error' and '-Wall' compile actions enabled
//

//=================================================================================================
// Clang
//=================================================================================================
#if defined (__clang__)
	#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
	#pragma clang diagnostic ignored "-Wmissing-prototypes"
	#pragma clang diagnostic ignored "-Wnonportable-system-include-path"
	#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
	#pragma clang diagnostic ignored "-Wold-style-cast"
	#pragma clang diagnostic ignored "-Wextra-semi-stmt"
	#pragma clang diagnostic ignored "-Wmissing-prototypes"
	#pragma clang diagnostic ignored "-Wsign-conversion"
	#pragma clang diagnostic ignored "-Wlanguage-extension-token"

//=================================================================================================
// Visual Studio warnings
//=================================================================================================
#elif defined(_MSC_VER) 
	#pragma warning (disable: 4061)		// warning C4061: enumerator xxx in switch of enum yyy is not explicitly handled by a case label (d3d11.h)
	#pragma warning (disable: 4191)		// warning C4191: 'type cast': unsafe conversion from 'xxx' to 'yyy'    
	#pragma warning (disable: 4365)		// warning C4365 : '=' : conversion from xxx to yyy, signed / unsigned mismatch	
	#pragma warning (disable: 4464)		// warning C4464: relative include path contains '..'	
	#pragma warning (disable: 4514)		// unreferenced inline function has been removed
	#pragma warning (disable: 4668)		// warning C4668 : xxxx is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#pragma warning (disable: 4710)		// warning C4710: 'xxx': function not inlined
	#pragma warning (disable: 4711)		// warning C4711: function 'xxx' selected for automatic inline expansion
	#pragma warning (disable: 4820)		// warning C4820 : xxx : yyy bytes padding added after data member zzz	
	#pragma warning (disable: 5045)		// warning C5045 : Compiler will insert Spectre mitigation for memory load if / Qspectre switch specified
	
#endif
