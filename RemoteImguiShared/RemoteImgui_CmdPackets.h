#pragma once

#include "RemoteImgui_Shared.h"
#include "RemoteImgui_ImguiFrame.h"

namespace RmtImgui
{

enum eTexFormat : uint8_t { kTexFmtRGB, kTexFmtRGBA, kTexFmt_Invalid };

//SF Finish this pointer converter
template <typename TType>
struct OffsetPointer
{
	union
	{
		uint64_t	mDataOffset;
		TType		mpData;
	};
};

struct CmdHeader
{
	enum eCommands : uint16_t { kCmdInvalid, kCmdPing, kCmdDisconnect, kCmdVersion, kCmdTexture, kCmdInput, kCmdDrawFrame };
	CmdHeader(){};
	CmdHeader(CmdHeader::eCommands CmdType, uint32_t Size) : mType(CmdType), mSize(Size) {}
	uint32_t mType	= kCmdInvalid;
	uint32_t mSize	= 0;
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
	CmdHeader	mHeader		= CmdHeader(CmdHeader::kCmdInput, sizeof(CmdInput));
	ImguiInput	mInput;
};

struct alignas(8) CmdTexture
{		
	CmdHeader	mHeader			= CmdHeader(CmdHeader::kCmdTexture, sizeof(CmdTexture));
	uint8_t*	mpTextureData	= nullptr;
	uint64_t	mTextureId;
	uint16_t	mWidth;
	uint16_t	mHeight;
	eTexFormat	mFormat;
	//uint8_t		mPadding[3];
};

struct alignas(8) CmdDrawFrame
{
	CmdHeader	mHeader		= CmdHeader(CmdHeader::kCmdDrawFrame, sizeof(CmdDrawFrame));
	ImguiFrame*	mpFrameData = nullptr;
};

template <typename CommandStruct>
void DeserializeCommand(CommandStruct*& pOutCommand, void* pData)
{
	pOutCommand	= reinterpret_cast<CommandStruct*>(pData);
}

template <>
void DeserializeCommand(CmdTexture*& pOutCommand, void* pData);

template <>
void DeserializeCommand(CmdDrawFrame*& pOutCommand, void* pData);

}
