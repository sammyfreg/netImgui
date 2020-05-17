//
// Deactivate a few warnings to allow internal netImgui code to compile 
// with 'Warning as error' and '-Wall' compile actions enabled
//

//=================================================================================================
// Clang
//=================================================================================================
#if defined (__clang__)

	#pragma clang diagnostic push

	#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
	#pragma clang diagnostic ignored "-Wmissing-prototypes"

//=================================================================================================
// Visual Studio warnings
//=================================================================================================
#elif defined(_MSC_VER) && !defined(__clang__) 
	
	#pragma warning (disable: 5032)		// warning C5032: detected #pragma warning(push) with no corresponding #pragma warning(pop)	
	#pragma warning	(push)

	#pragma warning (disable: 5031)		// warning C5031: #pragma warning(pop): likely mismatch, popping warning state pushed in different file	
	#pragma warning (disable: 4464)		// warning C4464: relative include path contains '..'
	#pragma warning (disable: 4365)		// conversion from 'long' to 'unsigned int', signed/unsigned mismatch for <atomic>
	#pragma warning (disable: 4514)		// unreferenced inline function has been removed
	#pragma warning (disable: 5045)		// warning C5045 : Compiler will insert Spectre mitigation for memory load if / Qspectre switch specified

#endif

