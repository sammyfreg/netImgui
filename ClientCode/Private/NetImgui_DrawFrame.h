#pragma once

#include "NetImGui_Shared.h"

namespace NetImgui { namespace Internal
{
//SF move this?
struct ImguiVert
{
	enum Constants{ kUvRange_Min=-2, kUvRange_Max=2, kPosRange_Min=-256, kPosRange_Max=2048};
	uint16_t	mPos[2];	
	uint16_t	mUV[2];
	uint32_t	mColor;
};

struct ImguiDraw
{
	uint64_t	mTextureId;
	uint32_t	mIdxCount;
	uint32_t	mIdxOffset;	// In Bytes
	uint32_t	mVtxOffset;	// In ImguiVert Index	
	uint16_t	mIndexSize;	// 2, 4 bytes
	float		mClipRect[4];	
};

struct CmdDrawFrame* CreateCmdDrawDrame(const ImDrawData* pDearImguiData);

}} // namespace NetImgui::Internal
