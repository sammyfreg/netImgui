#pragma once

#include "NetImgui_Shared.h"
#include "NetImgui_CmdPackets_DrawFrame.h"

namespace NetImgui { namespace Internal
{

//Note: If updating any of these commands data structure, increase 'CmdVersion::eVersion'

struct CmdHeader
{
	enum class eCommands : uint8_t { Invalid, Ping, Disconnect, Version, Texture, Input, DrawFrame, Background };
				CmdHeader(){}
				CmdHeader(eCommands CmdType, uint16_t Size) : mSize(Size), mType(CmdType){}
	uint32_t	mSize		= 0;
	eCommands	mType		= eCommands::Invalid;
	uint8_t		mPadding[3]	= {0,0,0};
};

struct alignas(8) CmdPing
{
	CmdHeader mHeader = CmdHeader(CmdHeader::eCommands::Ping, sizeof(CmdPing));
};

struct alignas(8) CmdDisconnect
{
	CmdHeader mHeader = CmdHeader(CmdHeader::eCommands::Disconnect, sizeof(CmdDisconnect));
};

struct alignas(8) CmdVersion
{
	enum class eVersion : uint32_t
	{
		Initial				= 1,
		NewTextureFormat	= 2,
		ImguiVersionInfo	= 3,	// Added Dear Imgui/ NetImgui version info to 'CmdVersion'
		ServerRefactor		= 4,	// Change to 'CmdInput' and 'CmdVersion' store size of 'ImWchar' to make sure they are compatible
		BackgroundCmd		= 5,	// Added new command to control background appearance
		ClientName			= 6,	// Increase maximum allowed client name that a program can set
		DataCompression		= 7,	// Adding support for data compression between client/server. Simple low cost delta compressor (only send difference from previous frame)
		DataCompression2	= 8,	// Improvement to data compression (save corner position and use SoA for vertices data)
		VertexUVRange		= 9,	// Changed vertices UV value range to [0,1] for increased precision on large font texture
		// Insert new version here

		//--------------------------------
		_count,
		_current			= _count -1
	};

	CmdHeader	mHeader					= CmdHeader(CmdHeader::eCommands::Version, sizeof(CmdVersion));
	eVersion	mVersion				= eVersion::_current;
	char		mClientName[64]			= {};
	char		mImguiVerName[16]		= {IMGUI_VERSION};
	char		mNetImguiVerName[16]	= {NETIMGUI_VERSION};
	uint32_t	mImguiVerID				= IMGUI_VERSION_NUM;
	uint32_t	mNetImguiVerID			= NETIMGUI_VERSION_NUM;
	uint8_t		mWCharSize				= static_cast<uint8_t>(sizeof(ImWchar));
	char		PADDING[3];
};

struct alignas(8) CmdInput
{
	enum MouseButton // copy of ImGuiMouseButton_ from imgui.h
	{
		MouseButton_Left = 0,
		MouseButton_Right = 1,
		MouseButton_Middle = 2,
		MouseButton_COUNT = 5
	};

	static void setupKeyMap(int *keyMap);
	inline bool IsMouseButtonDown(MouseButton button) const;
	inline void SetMouseButtonDown(MouseButton button, bool isDown);

	inline bool checkShiftModifier() const;
	inline bool checkCtrlModifier() const;
	inline bool checkAltModifier() const;
	inline bool checkSuperModifier() const;

	inline bool getKeyDownLegacy(bool* keysArray, size_t arraySize) const; // for version before 1.87

	inline bool IsKeyDown(ImGuiKey gKey) const; // for version starts from 1.87
	inline void SetKeyDown(ImGuiKey gKey, bool isDown); // for version starts from 1.87

	CmdHeader						mHeader				= CmdHeader(CmdHeader::eCommands::Input, sizeof(CmdInput));
	uint16_t						mScreenSize[2]		= {};
	int16_t							mMousePos[2]		= {};
	float							mMouseWheelVert		= 0.f;
	float							mMouseWheelHoriz	= 0.f;
	ImWchar							mKeyChars[256];					// Input characters	
	uint16_t						mKeyCharCount		= 0;		// Number of valid input characters
	bool							mCompressionUse		= false;	// Server would like client to compress the communication data
	bool							mCompressionSkip	= false;	// Server forcing next client's frame data to be uncompressed
	uint8_t							PADDING[4]			= {};

private:
	bool checkKeyDown(ImGuiKey gKey, ImGuiKey namedKeyBegin) const;

	uint64_t						mPressedMouseButtonsMask = 0;
	uint64_t						mKeysDownMask[512 / 64];	// List of keys currently pressed (follow GuiKey or ImGuiKey (from 1.87 version))
};

struct alignas(8) CmdTexture
{		
	CmdHeader						mHeader			= CmdHeader(CmdHeader::eCommands::Texture, sizeof(CmdTexture));
	OffsetPointer<uint8_t>			mpTextureData;
	uint64_t						mTextureId		= 0;
	uint16_t						mWidth			= 0;
	uint16_t						mHeight			= 0;
	uint8_t							mFormat			= eTexFormat::kTexFmt_Invalid;	// eTexFormat
	uint8_t							PADDING[3]		= {};
};

struct alignas(8) CmdDrawFrame
{
	CmdHeader						mHeader				= CmdHeader(CmdHeader::eCommands::DrawFrame, sizeof(CmdDrawFrame));
	uint64_t						mFrameIndex			= 0;
	uint32_t						mMouseCursor		= 0;	// ImGuiMouseCursor value
	float							mDisplayArea[4]		= {};
	uint32_t						mIndiceByteSize		= 0;
	uint32_t						mDrawGroupCount		= 0;
	uint32_t						mTotalVerticeCount	= 0;
	uint32_t						mTotalIndiceCount	= 0;
	uint32_t						mTotalDrawCount		= 0;
	uint32_t						mUncompressedSize	= 0;
	uint8_t							mCompressed			= false;
	uint8_t							PADDING[3];
	OffsetPointer<ImguiDrawGroup>	mpDrawGroups;
	inline void						ToPointers();
	inline void						ToOffsets();
};

struct alignas(8) CmdBackground
{
	static constexpr uint64_t		kDefaultTexture		= ~0u;
	CmdHeader						mHeader				= CmdHeader(CmdHeader::eCommands::Background, sizeof(CmdBackground));
	float							mClearColor[4]		= {0.2f, 0.2f, 0.2f, 1.f};	// Background color 
	float							mTextureTint[4]		= {1.f, 1.f, 1.f, 0.5f};	// Tint/alpha applied to texture
	uint64_t						mTextureId			= kDefaultTexture;			// Texture rendered in background, use server texture by default
	inline bool operator==(const CmdBackground& cmp)const;
	inline bool operator!=(const CmdBackground& cmp)const;
};


}} // namespace NetImgui::Internal

#include "NetImgui_CmdPackets.inl"
