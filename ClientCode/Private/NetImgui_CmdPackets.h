#pragma once

#include "NetImGui_Shared.h"
#include "NetImGui_DrawFrame.h"

namespace NetImgui { namespace Internal
{

struct CmdHeader
{
	enum eCommands : uint8_t { kCmdInvalid, kCmdPing, kCmdDisconnect, kCmdVersion, kCmdTexture, kCmdInput, kCmdDrawFrame };
				CmdHeader(){};
				CmdHeader(CmdHeader::eCommands CmdType, uint16_t Size) : mSize(Size), mType(CmdType) {}	
	uint32_t	mSize		= 0;
	eCommands	mType		= kCmdInvalid;
	uint8_t		mPadding[3]	= {0,0,0};
};

struct alignas(8) CmdPing
{
	CmdHeader mHeader = CmdHeader(CmdHeader::kCmdPing, sizeof(CmdPing));
};

struct alignas(8) CmdDisconnect
{
	CmdHeader mHeader = CmdHeader(CmdHeader::kCmdDisconnect, sizeof(CmdDisconnect));
};

struct alignas(8) CmdVersion
{
	enum eVersion : uint32_t
	{
		kVer_Initial = 1,
		// Insert new version here

		//--------------------------------
		kVer_Count,
		kVer_Current = kVer_Count -1
	};

	CmdHeader	mHeader			= CmdHeader(CmdHeader::kCmdVersion, sizeof(CmdVersion));
	eVersion	mVersion		= kVer_Current;
	char		mClientName[16];
	char		mPad[4];
};

struct alignas(8) CmdInput
{
	// From Windows Virtual Keys Code
	// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	enum eVirtualKeys{ 
		vkMouseBtnLeft		= 0x01,
		vkMouseBtnRight		= 0x02,
		vkMouseBtnMid		= 0x04,
		vkKeyboardShift		= 0x10,
		vkKeyboardCtrl		= 0x11,
		vkKeyboardAlt		= 0x12,
		vkKeyboardTab		= 0x09,
		vkKeyboardLeft		= 0x25,
		vkKeyboardRight		= 0x27,
		vkKeyboardUp		= 0x26,
		vkKeyboardDown		= 0x28,
		vkKeyboardPageUp	= 0x21,
		vkKeyboardPageDown	= 0x22,
		vkKeyboardHome		= 0x24,
		vkKeyboardEnd		= 0x23,
		vkKeyboardDel		= 0x2E,
		vkKeyboardBackspace	= 0x08,
		vkKeyboardEnter		= 0x0D,
		vkKeyboardEscape	= 0x1B,
	};
	inline bool IsKeyDown(eVirtualKeys vkKey)const;

	CmdHeader	mHeader		= CmdHeader(CmdHeader::kCmdInput, sizeof(CmdInput));
	uint16_t	ScreenSize[2];
	int16_t		MousePos[2];	
	float		MouseWheel;
	uint16_t	KeyChars[64];			// Input characters
	uint8_t		KeyCharCount;			// Number of valid input characters
	uint64_t	KeysDownMask[256/64];	// List of keys currently pressed (follow Windows Virtual-Key codes)
	
};

struct alignas(8) CmdTexture
{		
	CmdHeader				mHeader			= CmdHeader(CmdHeader::kCmdTexture, sizeof(CmdTexture));
	OffsetPointer<uint8_t>	mpTextureData;
	uint64_t				mTextureId;
	uint16_t				mWidth;
	uint16_t				mHeight;
	eTexFormat				mFormat;
};

struct alignas(8) CmdDrawFrame
{
	CmdHeader					mHeader		= CmdHeader(CmdHeader::kCmdDrawFrame, sizeof(CmdDrawFrame));	
	uint32_t					mVerticeCount;		
	uint32_t					mIndiceByteSize;	
	uint32_t					mDrawCount;
	OffsetPointer<ImguiVert>	mpVertices;
	OffsetPointer<uint8_t>		mpIndices;
	OffsetPointer<ImguiDraw>	mpDraws;
	inline void					ToPointers();
	inline void					ToOffsets();
};

}} // namespace NetImgui::Internal

#include "NetImgui_CmdPackets.inl"
