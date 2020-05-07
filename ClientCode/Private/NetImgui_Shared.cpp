#include "NetImGui_Shared.h"

namespace NetImgui { namespace Internal
{
	void* (*gpMalloc)(size_t)	= nullptr;
	void (*gpFree)(void*)		= nullptr;
		
}} //namespace NetImgui::Internal
