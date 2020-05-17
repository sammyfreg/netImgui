#pragma once

#include <stdint.h>
#include <vector>

namespace NetImgui { namespace Internal { struct CmdDrawFrame;  struct CmdTexture; } }

namespace dx
{

struct TextureHandle
{
				TextureHandle(): mImguiId(0), mIndex(static_cast<uint64_t>(-1)){}	
	inline bool IsValid(){ return mIndex != static_cast<uint64_t>(-1); }
	inline void SetInvalid(){ mIndex = static_cast<uint64_t>(-1); }
	uint64_t	mImguiId;
	uint64_t	mIndex;
};

bool			Startup(HWND hWindow);
void			Shutdown();
void			Render(const std::vector<TextureHandle>& textures, const NetImgui::Internal::CmdDrawFrame* pDrawFrame);

TextureHandle	TextureCreate(NetImgui::Internal::CmdTexture* pCmdTexture);
void			TextureRelease(TextureHandle& hTexture);

}
