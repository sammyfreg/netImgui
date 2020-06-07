#pragma once

#include "NetImgui_Shared.h"
#include "NetImgui_CmdPackets_DrawFrame.h"

namespace NetImgui { namespace Internal
{

//Note: If updating any of these commands data structure, increase 'CmdVersion::eVersion'

struct CmdHeader
{
	enum eCommands : uint8_t { kCmdInvalid, kCmdPing, kCmdDisconnect, kCmdVersion, kCmdTexture, kCmdInput, kCmdDrawFrame };
				CmdHeader(){}
				CmdHeader(CmdHeader::eCommands CmdType, uint16_t Size) : mSize(Size), mType(CmdType){}
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
		vkMouseBtnExtra1	= 0x05, // VK_XBUTTON1
		vkMouseBtnExtra2	= 0x06, // VK_XBUTTON2
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
		vkKeyboardSuper1	= 0x5B, // VK_LWIN
		vkKeyboardSuper2	= 0x5C, // VK_LWIN
	};
	inline bool IsKeyDown(eVirtualKeys vkKey)const;

	CmdHeader	mHeader		= CmdHeader(CmdHeader::kCmdInput, sizeof(CmdInput));
	uint16_t	mScreenSize[2];
	int16_t		mMousePos[2];	
	float		mMouseWheelVert;
	float		mMouseWheelHoriz;
	uint16_t	mKeyChars[64];			// Input characters	
	uint64_t	mKeysDownMask[256/64];	// List of keys currently pressed (follow Windows Virtual-Key codes)
	uint8_t		mKeyCharCount;			// Number of valid input characters
	uint8_t		PADDING[7];
};

struct alignas(8) CmdTexture
{		
	CmdHeader				mHeader			= CmdHeader(CmdHeader::kCmdTexture, sizeof(CmdTexture));
	OffsetPointer<uint8_t>	mpTextureData;
	uint64_t				mTextureId;
	uint16_t				mWidth;
	uint16_t				mHeight;
	uint8_t					mFormat;			// eTexFormat
	uint8_t					PADDING[3];
};

struct alignas(8) CmdDrawFrame
{
	CmdHeader					mHeader		= CmdHeader(CmdHeader::kCmdDrawFrame, sizeof(CmdDrawFrame));	
	uint32_t					mVerticeCount;		
	uint32_t					mIndiceByteSize;	
	uint32_t					mDrawCount;
	uint32_t					mMouseCursor;	// ImGuiMouseCursor value
	float						mDisplayArea[4];
	OffsetPointer<ImguiVert>	mpVertices;
	OffsetPointer<uint8_t>		mpIndices;
	OffsetPointer<ImguiDraw>	mpDraws;
	inline void					ToPointers();
	inline void					ToOffsets();
};

}} // namespace NetImgui::Internal

#include "NetImgui_CmdPackets.inl"
