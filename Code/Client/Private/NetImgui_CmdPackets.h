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
		Imgui_1_87			= 10,	// Added Dear ImGui Input refactor
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
	// Identify a mouse button.
	// Those values are guaranteed to be stable and we frequently use 0/1 directly. Named enums provided for convenience.
	enum NetImguiMouseButton
	{
		ImGuiMouseButton_Left = 0,
		ImGuiMouseButton_Right = 1,
		ImGuiMouseButton_Middle = 2,
		ImGuiMouseButton_Extra1 = 3,    // Additional entry
		ImGuiMouseButton_Extra2 = 4,    // Additional entry 
		ImGuiMouseButton_COUNT = 5
	};

	// Copy of Dear ImGui key enum
	// We keep our own internal version, to make sure Client key is the same as Server Key (since they can have different Imgui version)
	enum NetImguiKeys
	{
		// Keyboard
		ImGuiKey_Tab,
		ImGuiKey_LeftArrow,
		ImGuiKey_RightArrow,
		ImGuiKey_UpArrow,
		ImGuiKey_DownArrow,
		ImGuiKey_PageUp,
		ImGuiKey_PageDown,
		ImGuiKey_Home,
		ImGuiKey_End,
		ImGuiKey_Insert,
		ImGuiKey_Delete,
		ImGuiKey_Backspace,
		ImGuiKey_Space,
		ImGuiKey_Enter,
		ImGuiKey_Escape,
		ImGuiKey_LeftCtrl, ImGuiKey_LeftShift, ImGuiKey_LeftAlt, ImGuiKey_LeftSuper,
		ImGuiKey_RightCtrl, ImGuiKey_RightShift, ImGuiKey_RightAlt, ImGuiKey_RightSuper,
		ImGuiKey_Menu,
		ImGuiKey_0, ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4, ImGuiKey_5, ImGuiKey_6, ImGuiKey_7, ImGuiKey_8, ImGuiKey_9,
		ImGuiKey_A, ImGuiKey_B, ImGuiKey_C, ImGuiKey_D, ImGuiKey_E, ImGuiKey_F, ImGuiKey_G, ImGuiKey_H, ImGuiKey_I, ImGuiKey_J,
		ImGuiKey_K, ImGuiKey_L, ImGuiKey_M, ImGuiKey_N, ImGuiKey_O, ImGuiKey_P, ImGuiKey_Q, ImGuiKey_R, ImGuiKey_S, ImGuiKey_T,
		ImGuiKey_U, ImGuiKey_V, ImGuiKey_W, ImGuiKey_X, ImGuiKey_Y, ImGuiKey_Z,
		ImGuiKey_F1, ImGuiKey_F2, ImGuiKey_F3, ImGuiKey_F4, ImGuiKey_F5, ImGuiKey_F6,
		ImGuiKey_F7, ImGuiKey_F8, ImGuiKey_F9, ImGuiKey_F10, ImGuiKey_F11, ImGuiKey_F12,
		ImGuiKey_Apostrophe,        // '
		ImGuiKey_Comma,             // ,
		ImGuiKey_Minus,             // -
		ImGuiKey_Period,            // .
		ImGuiKey_Slash,             // /
		ImGuiKey_Semicolon,         // ;
		ImGuiKey_Equal,             // =
		ImGuiKey_LeftBracket,       // [
		ImGuiKey_Backslash,         // \ (this text inhibit multiline comment caused by backslash)
		ImGuiKey_RightBracket,      // ]
		ImGuiKey_GraveAccent,       // `
		ImGuiKey_CapsLock,
		ImGuiKey_ScrollLock,
		ImGuiKey_NumLock,
		ImGuiKey_PrintScreen,
		ImGuiKey_Pause,
		ImGuiKey_Keypad0, ImGuiKey_Keypad1, ImGuiKey_Keypad2, ImGuiKey_Keypad3, ImGuiKey_Keypad4,
		ImGuiKey_Keypad5, ImGuiKey_Keypad6, ImGuiKey_Keypad7, ImGuiKey_Keypad8, ImGuiKey_Keypad9,
		ImGuiKey_KeypadDecimal,
		ImGuiKey_KeypadDivide,
		ImGuiKey_KeypadMultiply,
		ImGuiKey_KeypadSubtract,
		ImGuiKey_KeypadAdd,
		ImGuiKey_KeypadEnter,
		ImGuiKey_KeypadEqual,

		// Gamepad (some of those are analog values, 0.0f to 1.0f)                              // NAVIGATION action
		ImGuiKey_GamepadStart,          // Menu (Xbox)          + (Switch)   Start/Options (PS) // --
		ImGuiKey_GamepadBack,           // View (Xbox)          - (Switch)   Share (PS)         // --
		ImGuiKey_GamepadFaceUp,         // Y (Xbox)             X (Switch)   Triangle (PS)      // -> ImGuiNavInput_Input
		ImGuiKey_GamepadFaceDown,       // A (Xbox)             B (Switch)   Cross (PS)         // -> ImGuiNavInput_Activate
		ImGuiKey_GamepadFaceLeft,       // X (Xbox)             Y (Switch)   Square (PS)        // -> ImGuiNavInput_Menu
		ImGuiKey_GamepadFaceRight,      // B (Xbox)             A (Switch)   Circle (PS)        // -> ImGuiNavInput_Cancel
		ImGuiKey_GamepadDpadUp,         // D-pad Up                                             // -> ImGuiNavInput_DpadUp
		ImGuiKey_GamepadDpadDown,       // D-pad Down                                           // -> ImGuiNavInput_DpadDown
		ImGuiKey_GamepadDpadLeft,       // D-pad Left                                           // -> ImGuiNavInput_DpadLeft
		ImGuiKey_GamepadDpadRight,      // D-pad Right                                          // -> ImGuiNavInput_DpadRight
		ImGuiKey_GamepadL1,             // L Bumper (Xbox)      L (Switch)   L1 (PS)            // -> ImGuiNavInput_FocusPrev + ImGuiNavInput_TweakSlow
		ImGuiKey_GamepadR1,             // R Bumper (Xbox)      R (Switch)   R1 (PS)            // -> ImGuiNavInput_FocusNext + ImGuiNavInput_TweakFast
		ImGuiKey_GamepadL2,             // L Trigger (Xbox)     ZL (Switch)  L2 (PS) [Analog]
		ImGuiKey_GamepadR2,             // R Trigger (Xbox)     ZR (Switch)  R2 (PS) [Analog]
		ImGuiKey_GamepadL3,             // L Thumbstick (Xbox)  L3 (Switch)  L3 (PS)
		ImGuiKey_GamepadR3,             // R Thumbstick (Xbox)  R3 (Switch)  R3 (PS)
		ImGuiKey_GamepadLStickUp,       // [Analog]                                             // -> ImGuiNavInput_LStickUp
		ImGuiKey_GamepadLStickDown,     // [Analog]                                             // -> ImGuiNavInput_LStickDown
		ImGuiKey_GamepadLStickLeft,     // [Analog]                                             // -> ImGuiNavInput_LStickLeft
		ImGuiKey_GamepadLStickRight,    // [Analog]                                             // -> ImGuiNavInput_LStickRight
		ImGuiKey_GamepadRStickUp,       // [Analog]
		ImGuiKey_GamepadRStickDown,     // [Analog]
		ImGuiKey_GamepadRStickLeft,     // [Analog]
		ImGuiKey_GamepadRStickRight,    // [Analog]

		// Keyboard Modifiers (explicitly submitted by backend via AddKeyEvent() calls)
		// - This is mirroring the data also written to io.KeyCtrl, io.KeyShift, io.KeyAlt, io.KeySuper, in a format allowing
		//   them to be accessed via standard key API, allowing calls such as IsKeyPressed(), IsKeyReleased(), querying duration etc.
		// - Code polling every keys (e.g. an interface to detect a key press for input mapping) might want to ignore those
		//   and prefer using the real keys (e.g. ImGuiKey_LeftCtrl, ImGuiKey_RightCtrl instead of ImGuiKey_ModCtrl).
		// - In theory the value of keyboard modifiers should be roughly equivalent to a logical or of the equivalent left/right keys.
		//   In practice: it's complicated; mods are often provided from different sources. Keyboard layout, IME, sticky keys and
		//   backends tend to interfere and break that equivalence. The safer decision is to relay that ambiguity down to the end-user...
		ImGuiKey_ModCtrl, ImGuiKey_ModShift, ImGuiKey_ModAlt, ImGuiKey_ModSuper,

		// End of list
		ImGuiKey_COUNT,                 // No valid ImGuiKey is ever greater than this value
	};

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
	uint64_t						mMouseDownMask		= 0;
	uint64_t						mInputDownMask[(ImGuiKey_COUNT+63)/64];

	inline bool						IsKeyDown(NetImguiKeys netimguiKey) const;
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
