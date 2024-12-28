#pragma once

#include "NetImgui_Shared.h"
#include "NetImgui_CmdPackets_DrawFrame.h"

namespace NetImgui { namespace Internal
{

//Note: If updating any of these commands data structure, increase 'CmdVersion::eVersion'
struct alignas(8) CmdHeader
{
	enum class eCommands : uint8_t { Version, Texture, Input, DrawFrame, Background, Clipboard, Count };
				CmdHeader(eCommands CmdType, uint16_t Size) : mSize(Size), mType(CmdType){}
	uint32_t	mSize		= 0;
	eCommands	mType		= eCommands::Count;
	uint8_t		mPadding[3]	= {};
};

// Used as step 1 of 2 of reading incoming transmission between Client/Server, to get header whose size we know
struct alignas(8) CmdPendingRead : public CmdHeader
{
	CmdPendingRead() : CmdHeader(eCommands::Count, sizeof(CmdPendingRead) ){}
};

struct alignas(8) CmdVersion : public CmdHeader
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
		OffetPointer		= 11,	// Updated the handling of OffsetPoint. Moved flag bit from last bit to first bit. Addresses and data are always at least 4 bytes aligned, so should never conflict with potential address space
		CustomTexture		= 12,	// Added a 'custom' texture format to let user potentially handle their how format
		DPIScale			= 13,	// Server now handle monitor DPI
		Clipboard			= 14,	// Added clipboard support between server/client
		ForceReconnect		= 15,	// Server can now take over the connection from another server
		UpdatedComs 		= 16,	// Faster protocol by removing blocking coms
		RemDisconnect		= 17,	// Removed Disconnect command
		TextureUpdate 		= 18, 	// Adding support for new Dear Imgui Texture update
		// Insert new version here

		//--------------------------------
		_count,
		_current			= _count -1
	};
	enum class eFlags : uint8_t
	{
		IsUnavailable 		= 0x01,	// Client telling Server it cannot be used
		IsConnected 		= 0x02,	// Client telling Server there's already a valid connection (can potentially be taken over if !IsUnavailable)
		ConnectForce		= 0x04,	// Server telling Client it want to take over connection if there's already one
		ConnectExclusive	= 0x08,	// Server telling Client that once connected, others servers should be denied access
	};
	CmdVersion() : CmdHeader(CmdHeader::eCommands::Version, sizeof(CmdVersion)){}
	char		mClientName[64]			= {};
	char		mImguiVerName[16]		= {IMGUI_VERSION};
	char		mNetImguiVerName[16]	= {NETIMGUI_VERSION};
	eVersion	mVersion				= eVersion::_current;
	uint32_t	mImguiVerID				= IMGUI_VERSION_NUM;
	uint32_t	mNetImguiVerID			= NETIMGUI_VERSION_NUM;
	uint8_t		mWCharSize				= static_cast<uint8_t>(sizeof(ImWchar));
	uint8_t 	mFlags 					= 0;
	char		PADDING[2];
};

struct alignas(8) CmdInput : public CmdHeader
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
		ImGuiKey_F13, ImGuiKey_F14, ImGuiKey_F15, ImGuiKey_F16, ImGuiKey_F17, ImGuiKey_F18,
		ImGuiKey_F19, ImGuiKey_F20, ImGuiKey_F21, ImGuiKey_F22, ImGuiKey_F23, ImGuiKey_F24,
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
		ImGuiKey_KeypadDivide, ImGuiKey_KeypadMultiply, ImGuiKey_KeypadSubtract, 
		ImGuiKey_KeypadAdd, ImGuiKey_KeypadEnter, ImGuiKey_KeypadEqual,
		ImGuiKey_AppBack,               // Available on some keyboard/mouses. Often referred as "Browser Back"
		ImGuiKey_AppForward,

		// Gamepad (some of those are analog values, 0.0f to 1.0f)                          // GAME NAVIGATION ACTION
		// (download controller mapping PNG/PSD at http://dearimgui.org/controls_sheets)
		ImGuiKey_GamepadStart,          // Menu (Xbox)      + (Switch)   Start/Options (PS)
		ImGuiKey_GamepadBack,           // View (Xbox)      - (Switch)   Share (PS)
		ImGuiKey_GamepadFaceLeft,       // X (Xbox)         Y (Switch)   Square (PS)        // Tap: Toggle Menu. Hold: Windowing mode (Focus/Move/Resize windows)
		ImGuiKey_GamepadFaceRight,      // B (Xbox)         A (Switch)   Circle (PS)        // Cancel / Close / Exit
		ImGuiKey_GamepadFaceUp,         // Y (Xbox)         X (Switch)   Triangle (PS)      // Text Input / On-screen Keyboard
		ImGuiKey_GamepadFaceDown,       // A (Xbox)         B (Switch)   Cross (PS)         // Activate / Open / Toggle / Tweak
		ImGuiKey_GamepadDpadLeft,       // D-pad Left                                       // Move / Tweak / Resize Window (in Windowing mode)
		ImGuiKey_GamepadDpadRight,      // D-pad Right                                      // Move / Tweak / Resize Window (in Windowing mode)
		ImGuiKey_GamepadDpadUp,         // D-pad Up                                         // Move / Tweak / Resize Window (in Windowing mode)
		ImGuiKey_GamepadDpadDown,       // D-pad Down                                       // Move / Tweak / Resize Window (in Windowing mode)
		ImGuiKey_GamepadL1,             // L Bumper (Xbox)  L (Switch)   L1 (PS)            // Tweak Slower / Focus Previous (in Windowing mode)
		ImGuiKey_GamepadR1,             // R Bumper (Xbox)  R (Switch)   R1 (PS)            // Tweak Faster / Focus Next (in Windowing mode)
		ImGuiKey_GamepadL2,             // L Trig. (Xbox)   ZL (Switch)  L2 (PS) [Analog]
		ImGuiKey_GamepadR2,             // R Trig. (Xbox)   ZR (Switch)  R2 (PS) [Analog]
		ImGuiKey_GamepadL3,             // L Stick (Xbox)   L3 (Switch)  L3 (PS)
		ImGuiKey_GamepadR3,             // R Stick (Xbox)   R3 (Switch)  R3 (PS)

		ImGuiKey_GamepadLStickLeft,     // [Analog]                                         // Move Window (in Windowing mode)
		ImGuiKey_GamepadLStickRight,    // [Analog]                                         // Move Window (in Windowing mode)
		ImGuiKey_GamepadLStickUp,       // [Analog]                                         // Move Window (in Windowing mode)
		ImGuiKey_GamepadLStickDown,     // [Analog]                                         // Move Window (in Windowing mode)
		ImGuiKey_GamepadRStickLeft,     // [Analog]
		ImGuiKey_GamepadRStickRight,    // [Analog]
		ImGuiKey_GamepadRStickUp,       // [Analog]
		ImGuiKey_GamepadRStickDown,     // [Analog]

		// Mouse Buttons (auto-submitted from AddMouseButtonEvent() calls)
		// - This is mirroring the data also written to io.MouseDown[], io.MouseWheel, in a format allowing them to be accessed via standard key API.
		ImGuiKey_MouseLeft, ImGuiKey_MouseRight, ImGuiKey_MouseMiddle, ImGuiKey_MouseX1, ImGuiKey_MouseX2, ImGuiKey_MouseWheelX, ImGuiKey_MouseWheelY,

		// Keyboard Modifiers (explicitly submitted by backend via AddKeyEvent() calls)
		// - This is mirroring the data also written to io.KeyCtrl, io.KeyShift, io.KeyAlt, io.KeySuper, in a format allowing
		//   them to be accessed via standard key API, allowing calls such as IsKeyPressed(), IsKeyReleased(), querying duration etc.
		// - Code polling every keys (e.g. an interface to detect a key press for input mapping) might want to ignore those
		//   and prefer using the real keys (e.g. ImGuiKey_LeftCtrl, ImGuiKey_RightCtrl instead of ImGuiKey_ModCtrl).
		// - In theory the value of keyboard modifiers should be roughly equivalent to a logical or of the equivalent left/right keys.
		//   In practice: it's complicated; mods are often provided from different sources. Keyboard layout, IME, sticky keys and
		//   backends tend to interfere and break that equivalence. The safer decision is to relay that ambiguity down to the end-user...
		ImGuiKey_ReservedForModCtrl, ImGuiKey_ReservedForModShift, ImGuiKey_ReservedForModAlt, ImGuiKey_ReservedForModSuper,

		// End of list
		ImGuiKey_COUNT,                 // No valid ImGuiKey is ever greater than this value
	};

	
	static constexpr uint32_t kAnalog_First	= ImGuiKey_GamepadLStickLeft;
	static constexpr uint32_t kAnalog_Last	= ImGuiKey_GamepadRStickDown;
	static constexpr uint32_t kAnalog_Count	= kAnalog_Last - kAnalog_First + 1;

	CmdInput() : CmdHeader(CmdHeader::eCommands::Input, sizeof(CmdInput)){}
	uint16_t						mScreenSize[2]					= {};
	int16_t							mMousePos[2]					= {};
	float							mMouseWheelVert					= 0.f;
	float							mMouseWheelHoriz				= 0.f;
	uint16_t						mKeyChars[256]					= {};		// Input characters		
	uint16_t						mKeyCharCount					= 0;		// Number of valid input characters
	bool							mCompressionUse					= false;	// Server would like client to compress the communication data
	bool							mCompressionSkip				= false;	// Server forcing next client's frame data to be uncompressed
	float							mFontDPIScaling					= 1.f;		// Font scaling request by Server accounting for monitor DPI
	float 							mDesiredFps						= 30.f;		// Requested redraw speed
	uint64_t						mMouseDownMask					= 0;
	uint64_t						mInputDownMask[(ImGuiKey_COUNT+63)/64]={};
	float							mInputAnalog[kAnalog_Count]		= {};
	inline bool						IsKeyDown(NetImguiKeys netimguiKey) const;
};

struct alignas(8) CmdTexture : public CmdHeader
{
	CmdTexture() : CmdHeader(CmdHeader::eCommands::Texture, sizeof(CmdTexture)){}
	uint64_t						mTextureUserId		= 0;
	uint8_t							mFormat				= eTexFormat::kTexFmt_Invalid;	// eTexFormat
	uint8_t							mUpdatable			= false;						// Set to true on Create, for updatable textures
	uint8_t							PADDING[1]			= {};
	uint16_t						mWidth				= 0;
	uint16_t						mHeight				= 0;
	OffsetPointer<uint8_t>			mpTextureData;

#if 0
	OffsetPointer<uint8_t>			mpTextureData;
	uint64_t						mTextureId		= 0;
	uint16_t						mWidth			= 0;
	uint16_t						mHeight			= 0;
	uint8_t							mFormat			= eTexFormat::kTexFmt_Invalid;	// eTexFormat
	uint8_t							PADDING[3]		= {};


	ImTextureStatus     Status;                 // ImTextureStatus_OK/_WantCreate/_WantUpdates/_WantDestroy
    ImTextureFormat     Format;                 // ImTextureFormat_RGBA32 (default) or ImTextureFormat_Alpha8
    int                 Width;
    int                 Height;
    int                 BytesPerPixel;          // 4 or 1
    unsigned char*      Pixels;                 // Pointer to buffer holding Width*Height pixels
    ImTextureUserID     BackendTexID;           // Identifier stored in ImDrawCmd and passed to backend RenderDrawData loop. // FIXME-NEWATLAS: Confirm naming?
    void*               BackendUserData;        // Convenience storage for backend. Some backends may have enough with BackendTexID.
    int                 UnusedFrames;           // In order to facilitate handling Status==WantDestroy in some backend: this is a count successive frames where the texture was not used.
    ImVector<ImTextureDataUpdate> Updates;      // List of individual updates
    ImTextureDataUpdate UpdatesMerged;          // Bounding box encompassing all individual updates
#endif
};

struct alignas(8) CmdDrawFrame : public CmdHeader
{
	CmdDrawFrame() : CmdHeader(CmdHeader::eCommands::DrawFrame, sizeof(CmdDrawFrame)){}
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
	uint8_t							PADDING[3]			= {};
	OffsetPointer<ImguiDrawGroup>	mpDrawGroups;
	inline void						ToPointers();
	inline void						ToOffsets();
};

struct alignas(8) CmdBackground : public CmdHeader
{
	CmdBackground() : CmdHeader(CmdHeader::eCommands::Background, sizeof(CmdBackground)){}
	static constexpr uint64_t		kDefaultTexture		= ~0u;
	float							mClearColor[4]		= {0.2f, 0.2f, 0.2f, 1.f};	// Background color 
	float							mTextureTint[4]		= {1.f, 1.f, 1.f, 0.5f};	// Tint/alpha applied to texture
	uint64_t						mTextureId			= kDefaultTexture;			// Texture rendered in background, use server texture by default
	inline bool operator==(const CmdBackground& cmp)const;
	inline bool operator!=(const CmdBackground& cmp)const;
};

struct alignas(8) CmdClipboard : public CmdHeader
{
	CmdClipboard() : CmdHeader(CmdHeader::eCommands::Clipboard, sizeof(CmdClipboard)){}
	size_t							mByteSize			= 0;
	OffsetPointer<char>				mContentUTF8;
	inline void						ToPointers();
	inline void						ToOffsets();
	inline static CmdClipboard*		Create(const char* clipboard);
};

//=============================================================================
// Keeping track of partial incoming/outgoing transmissions
//=============================================================================
struct PendingCom
{
	size_t SizeCurrent	= 0;		// Amount of data sent or received so far
	bool bAutoFree		= false;	// Need to free data buffer at the end of processing
	bool bError			= false;	// If an error occurs during coms
	CmdHeader* pCommand	= nullptr;	// Where to store incoming data or read to send data
	inline bool IsError()const{ return bError; }
	inline bool IsDone()const { return IsError() || (pCommand && pCommand->mSize == SizeCurrent); }
	inline bool IsReady()const{ return !IsError() && pCommand == nullptr; }	
	inline bool IsPending()const{ return !IsError() && !IsDone() && !IsReady(); }
};

}} // namespace NetImgui::Internal

#include "NetImgui_CmdPackets.inl"
