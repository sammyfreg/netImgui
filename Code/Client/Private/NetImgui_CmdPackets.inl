#include "NetImgui_CmdPackets.h"

namespace {
	enum GuiKey //copy of ImGuiKey_ enum from 1.87 version
	{
		// Keyboard
		Key_None = 0,
		Key_Tab = 512,             // == ImGuiKey_NamedKey_BEGIN
		Key_LeftArrow,
		Key_RightArrow,
		Key_UpArrow,
		Key_DownArrow,
		Key_PageUp,
		Key_PageDown,
		Key_Home,
		Key_End,
		Key_Insert,
		Key_Delete,
		Key_Backspace,
		Key_Space,
		Key_Enter,
		Key_Escape,
		Key_LeftCtrl, Key_LeftShift, Key_LeftAlt, Key_LeftSuper,
		Key_RightCtrl, Key_RightShift, Key_RightAlt, Key_RightSuper,
		Key_Menu,
		Key_0, Key_1, Key_2, Key_3, Key_4, Key_5, Key_6, Key_7, Key_8, Key_9,
		Key_A, Key_B, Key_C, Key_D, Key_E, Key_F, Key_G, Key_H, Key_I, Key_J,
		Key_K, Key_L, Key_M, Key_N, Key_O, Key_P, Key_Q, Key_R, Key_S, Key_T,
		Key_U, Key_V, Key_W, Key_X, Key_Y, Key_Z,
		Key_F1, Key_F2, Key_F3, Key_F4, Key_F5, Key_F6,
		Key_F7, Key_F8, Key_F9, Key_F10, Key_F11, Key_F12,
		Key_Apostrophe,        // '
		Key_Comma,             // ,
		Key_Minus,             // -
		Key_Period,            // .
		Key_Slash,             // /
		Key_Semicolon,         // ;
		Key_Equal,             // =
		Key_LeftBracket,       // [
		Key_Backslash,         // \ (this text inhibit multiline comment caused by backslash)
		Key_RightBracket,      // ]
		Key_GraveAccent,       // `
		Key_CapsLock,
		Key_ScrollLock,
		Key_NumLock,
		Key_PrintScreen,
		Key_Pause,
		Key_Keypad0, Key_Keypad1, Key_Keypad2, Key_Keypad3, Key_Keypad4,
		Key_Keypad5, Key_Keypad6, Key_Keypad7, Key_Keypad8, Key_Keypad9,
		Key_KeypadDecimal,
		Key_KeypadDivide,
		Key_KeypadMultiply,
		Key_KeypadSubtract,
		Key_KeypadAdd,
		Key_KeypadEnter,
		Key_KeypadEqual,

		// Gamepad (some of those are analog values, 0.0f to 1.0f)                              // NAVIGATION action
		Key_GamepadStart,          // Menu (Xbox)          + (Switch)   Start/Options (PS) // --
		Key_GamepadBack,           // View (Xbox)          - (Switch)   Share (PS)         // --
		Key_GamepadFaceUp,         // Y (Xbox)             X (Switch)   Triangle (PS)      // -> ImGuiNavInput_Input
		Key_GamepadFaceDown,       // A (Xbox)             B (Switch)   Cross (PS)         // -> ImGuiNavInput_Activate
		Key_GamepadFaceLeft,       // X (Xbox)             Y (Switch)   Square (PS)        // -> ImGuiNavInput_Menu
		Key_GamepadFaceRight,      // B (Xbox)             A (Switch)   Circle (PS)        // -> ImGuiNavInput_Cancel
		Key_GamepadDpadUp,         // D-pad Up                                             // -> ImGuiNavInput_DpadUp
		Key_GamepadDpadDown,       // D-pad Down                                           // -> ImGuiNavInput_DpadDown
		Key_GamepadDpadLeft,       // D-pad Left                                           // -> ImGuiNavInput_DpadLeft
		Key_GamepadDpadRight,      // D-pad Right                                          // -> ImGuiNavInput_DpadRight
		Key_GamepadL1,             // L Bumper (Xbox)      L (Switch)   L1 (PS)            // -> ImGuiNavInput_FocusPrev + ImGuiNavInput_TweakSlow
		Key_GamepadR1,             // R Bumper (Xbox)      R (Switch)   R1 (PS)            // -> ImGuiNavInput_FocusNext + ImGuiNavInput_TweakFast
		Key_GamepadL2,             // L Trigger (Xbox)     ZL (Switch)  L2 (PS) [Analog]
		Key_GamepadR2,             // R Trigger (Xbox)     ZR (Switch)  R2 (PS) [Analog]
		Key_GamepadL3,             // L Thumbstick (Xbox)  L3 (Switch)  L3 (PS)
		Key_GamepadR3,             // R Thumbstick (Xbox)  R3 (Switch)  R3 (PS)
		Key_GamepadLStickUp,       // [Analog]                                             // -> ImGuiNavInput_LStickUp
		Key_GamepadLStickDown,     // [Analog]                                             // -> ImGuiNavInput_LStickDown
		Key_GamepadLStickLeft,     // [Analog]                                             // -> ImGuiNavInput_LStickLeft
		Key_GamepadLStickRight,    // [Analog]                                             // -> ImGuiNavInput_LStickRight
		Key_GamepadRStickUp,       // [Analog]
		Key_GamepadRStickDown,     // [Analog]
		Key_GamepadRStickLeft,     // [Analog]
		Key_GamepadRStickRight,    // [Analog]

		// Keyboard Modifiers (explicitly submitted by backend via AddKeyEvent() calls)
		// - This is mirroring the data also written to io.KeyCtrl, io.KeyShift, io.KeyAlt, io.KeySuper, in a format allowing
		//   them to be accessed via standard key API, allowing calls such as IsKeyPressed(), IsKeyReleased(), querying duration etc.
		// - Code polling every keys (e.g. an interface to detect a key press for input mapping) might want to ignore those
		//   and prefer using the real keys (e.g. ImGuiKey_LeftCtrl, ImGuiKey_RightCtrl instead of ImGuiKey_ModCtrl).
		// - In theory the value of keyboard modifiers should be roughly equivalent to a logical or of the equivalent left/right keys.
		//   In practice: it's complicated; mods are often provided from different sources. Keyboard layout, IME, sticky keys and
		//   backends tend to interfere and break that equivalence. The safer decision is to relay that ambiguity down to the end-user...
		Key_ModCtrl, Key_ModShift, Key_ModAlt, Key_ModSuper,

		// End of list
		Key_COUNT,                 // No valid ImGuiKey is ever greater than this value

		// [Internal] Prior to 1.87 we required user to fill io.KeysDown[512] using their own native index + a io.KeyMap[] array.
		// We are ditching this method but keeping a legacy path for user code doing e.g. IsKeyPressed(MY_NATIVE_KEY_CODE)
		Key_NamedKey_BEGIN = 512,
		Key_NamedKey_END = Key_COUNT,
		Key_NamedKey_COUNT = Key_NamedKey_END - Key_NamedKey_BEGIN,
#ifdef IMGUI_DISABLE_OBSOLETE_KEYIO
		Key_KeysData_SIZE = Key_NamedKey_COUNT,          // Size of KeysData[]: only hold named keys
		Key_KeysData_OFFSET = Key_NamedKey_BEGIN           // First key stored in KeysData[0]
#else
		Key_KeysData_SIZE = Key_COUNT,                   // Size of KeysData[]: hold legacy 0..512 keycodes + named keys
		Key_KeysData_OFFSET = 0                                 // First key stored in KeysData[0]
#endif

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
		, Key_KeyPadEnter = Key_KeypadEnter   // Renamed in 1.87
#endif
	};
}

namespace NetImgui { namespace Internal
{
// @sammyfreg TODO: Make Offset/Pointer test safer
void CmdDrawFrame::ToPointers()
{
	if( !mpDrawGroups.IsPointer() )
	{
		mpDrawGroups.ToPointer();
		for (uint32_t i(0); i < mDrawGroupCount; ++i) {
			mpDrawGroups[i].ToPointers();
		}
	}
}

void CmdDrawFrame::ToOffsets()
{
	if( !mpDrawGroups.IsOffset() )
	{
		for (uint32_t i(0); i < mDrawGroupCount; ++i) {
			mpDrawGroups[i].ToOffsets();
		}
		mpDrawGroups.ToOffset();
	}
}

void ImguiDrawGroup::ToPointers()
{
	if( !mpIndices.IsPointer() ) //Safer to test the first element after CmdHeader
	{
		mpIndices.ToPointer();
		mpVertices.ToPointer();
		mpDraws.ToPointer();
	}
}

void ImguiDrawGroup::ToOffsets()
{
	if( !mpIndices.IsOffset() ) //Safer to test the first element after CmdHeader
	{
		mpIndices.ToOffset();
		mpVertices.ToOffset();
		mpDraws.ToOffset();
	}
}

inline void CmdInput::setupKeyMap(int* keyMap)
{
	keyMap[ImGuiKey_Tab] = static_cast<int>(Key_Tab);
	keyMap[ImGuiKey_LeftArrow] = static_cast<int>(Key_LeftArrow);
	keyMap[ImGuiKey_RightArrow] = static_cast<int>(Key_RightArrow);
	keyMap[ImGuiKey_UpArrow] = static_cast<int>(Key_UpArrow);
	keyMap[ImGuiKey_DownArrow] = static_cast<int>(Key_DownArrow);
	keyMap[ImGuiKey_PageUp] = static_cast<int>(Key_PageUp);
	keyMap[ImGuiKey_PageDown] = static_cast<int>(Key_PageDown);
	keyMap[ImGuiKey_Home] = static_cast<int>(Key_Home);
	keyMap[ImGuiKey_End] = static_cast<int>(Key_End);
	keyMap[ImGuiKey_Insert] = static_cast<int>(Key_Insert);
	keyMap[ImGuiKey_Delete] = static_cast<int>(Key_Delete);
	keyMap[ImGuiKey_Backspace] = static_cast<int>(Key_Backspace);
	keyMap[ImGuiKey_Space] = static_cast<int>(Key_Space);
	keyMap[ImGuiKey_Enter] = static_cast<int>(Key_Enter);
	keyMap[ImGuiKey_Escape] = static_cast<int>(Key_Escape);
	keyMap[ImGuiKey_A] = static_cast<int>(Key_A);
	keyMap[ImGuiKey_C] = static_cast<int>(Key_C);
	keyMap[ImGuiKey_V] = static_cast<int>(Key_V);
	keyMap[ImGuiKey_X] = static_cast<int>(Key_X);
	keyMap[ImGuiKey_Y] = static_cast<int>(Key_Y);
	keyMap[ImGuiKey_Z] = static_cast<int>(Key_Z);
}

bool CmdInput::IsMouseButtonDown(ImGuiMouseButton button)const
{
	if (button >= ImGuiMouseButton_COUNT) {
		return false;
	}

	return mPressedMouseButtonsMask & (uint64_t(1) << button);
}

void CmdInput::SetMouseButtonDown(ImGuiMouseButton button, bool isDown)
{
	if (button >= ImGuiMouseButton_COUNT) {
		return;
	}

	const uint64_t maskValue = uint64_t(1) << button;
	if (isDown) {
		mPressedMouseButtonsMask |= maskValue;
	}
	else {
		mPressedMouseButtonsMask &= ~maskValue;
	}
}

inline bool CmdInput::getKeyDownLegacy(bool *keysArray, size_t arraySize) const
{
	if (arraySize != 512) {
		return false;
	}

	for (uint32_t i = 0; i < arraySize; ++i) {
		const uint64_t key = static_cast<uint64_t>(i);
		keysArray[i] = (mKeysDownMask[key / 64] & (static_cast<uint64_t>(1) << (key % 64))) != 0;
	}

	return true;
}

bool CmdInput::IsKeyDown(ImGuiKey gKey) const
{
	return checkKeyDown(gKey, ImGuiKey_Tab);
}

inline bool CmdInput::checkShiftModifier() const
{
	return checkKeyDown(Key_ModShift, Key_NamedKey_BEGIN);
}

inline bool CmdInput::checkCtrlModifier() const
{
	return checkKeyDown(Key_ModCtrl, Key_NamedKey_BEGIN);
}

inline bool CmdInput::checkAltModifier() const
{
	return checkKeyDown(Key_ModAlt, Key_NamedKey_BEGIN);
}

inline bool CmdInput::checkSuperModifier() const
{
	return checkKeyDown(Key_ModSuper, Key_NamedKey_BEGIN);
}

void CmdInput::SetKeyDown(ImGuiKey gKey, bool isDown)
{
	const ImGuiKey namedKeyBegin = ImGuiKey_Tab;
	if (gKey >= ImGuiKey_COUNT || gKey < namedKeyBegin) {
		return;
	}

	const uint64_t relateIndex = uint64_t(gKey) - namedKeyBegin;
	const uint64_t keyEntryIndex = static_cast<uint64_t>(relateIndex) / 64;

	if (keyEntryIndex >= 512 / 64) {
		return;
	}

	const uint64_t keyBitMask = static_cast<uint64_t>(1) << static_cast<uint64_t>(relateIndex) % 64;
	mKeysDownMask[keyEntryIndex]= isDown ? mKeysDownMask[keyEntryIndex] | keyBitMask :
										   mKeysDownMask[keyEntryIndex] & ~keyBitMask;
}

inline bool CmdInput::checkKeyDown(ImGuiKey gKey, ImGuiKey namedKeyBegin) const
{
	const uint64_t relateIndex = uint64_t(gKey) - namedKeyBegin;
	const uint64_t keyEntryIndex = static_cast<uint64_t>(relateIndex) / 64;

	if (keyEntryIndex >= 512 / 64) {
		return false;
	}

	return (mKeysDownMask[keyEntryIndex] & (static_cast<uint64_t>(1) << (relateIndex % 64))) != 0;
}

bool CmdBackground::operator==(const CmdBackground& cmp)const
{
	bool sameValue(true);
	for(size_t i(0); i<sizeof(CmdBackground)/8; i++){
		sameValue &= reinterpret_cast<const uint64_t*>(this)[i] == reinterpret_cast<const uint64_t*>(&cmp)[i];
	}
	return sameValue;
}

bool CmdBackground::operator!=(const CmdBackground& cmp)const
{
	return (*this == cmp) == false;
}

}} // namespace NetImgui::Internal
