#pragma once

#include <stdint.h>
#include <vector>

namespace RmtImgui
{ 
	struct ImguiFrame; 
	struct CmdTexture;
}

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
void			Render(const std::vector<TextureHandle>& textures, const RmtImgui::ImguiFrame* pDrawInfo);

TextureHandle	TextureCreate(const RmtImgui::CmdTexture* pCmdTexture);
void			TextureRelease(const TextureHandle& hTexture);

}
