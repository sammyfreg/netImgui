#pragma once

#include "RemoteImgui_Shared.h"

struct ImDrawData;
namespace RmtImgui
{

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

struct ImguiFrame
{
	uint32_t	mDataSize;		// In Bytes
	uint32_t	mVerticeCount;	// In ImguiVert Count
	uint32_t	mIndiceSize;	// In Bytes
	uint32_t	mDrawCount;		// In ImguiDraw Count

	union{ uint64_t mOffsetVertice; ImguiVert* mpVertices;};	// Offset(disk) or Pointer(loaded) to Vertices data			
	union{ uint64_t mOffsetIndice;	uint8_t* mpIndices;};		// Offset(disk) or Pointer(loaded) to Indices data
	union{ uint64_t mOffsetDraw;	ImguiDraw* mpDraws;};		// Offset(disk) or Pointer(loaded) to Draw data
	
	void				UpdatePointer();
	static ImguiFrame*	Create(const ImDrawData* pDearImguiData);
};

//SF Move that directly in InputCommand?
struct ImguiInput
{
	enum eMouseBtn{ kMouseBtn_Left=1<<0, kMouseBtn_Right=1<<1, kMouseBtn_Middle=1<<2 };
	uint16_t	ScreenSize[2];
	int16_t		MousePos[2];	
	float		MouseWheel;
	uint16_t	KeyChars[64];			// Input characters
	uint8_t		KeyCharCount;			// Number of valid input characters
	uint8_t		MouseDownMask;			// Mask of pressed button
	uint64_t	KeysDownMask[256/8];	// List of keys currently pressed (follow Windows Virtual-Key codes)
};

} // namespace RmtImgui