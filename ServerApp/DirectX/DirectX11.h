#pragma once

#include <stdint.h>
#include <vector>

namespace NetImgui { namespace Internal { struct CmdDrawFrame;  struct CmdTexture; } }

namespace dx
{

struct TextureHandle
{
	uint64_t mImguiId	= 0;
	uint64_t mIndex		= (uint64_t)-1;
	inline bool IsValid(){ return mIndex != (uint64_t)-1; }
};

bool			Startup(HWND hWindow);
void			Shutdown();
void			Render(const std::vector<TextureHandle>& textures, const NetImgui::Internal::CmdDrawFrame* pDrawFrame);

TextureHandle	TextureCreate(NetImgui::Internal::CmdTexture* pCmdTexture);
void			TextureRelease(const TextureHandle& hTexture);

}
