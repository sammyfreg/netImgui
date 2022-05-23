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

#if IMGUI_VERSION_NUM < 18700

bool CmdInput::IsKeyDown(eVirtualKeys vkKey)const
{
	const uint64_t key = static_cast<uint64_t>(vkKey);
	return (mKeysDownMask[key/64] & (static_cast<uint64_t>(1)<<(key%64))) != 0;
}

void CmdInput::SetKeyDown(eVirtualKeys vkKey, bool isDown)
{
	const uint64_t keyEntryIndex	= static_cast<uint64_t>(vkKey) / 64;
	const uint64_t keyBitMask		= static_cast<uint64_t>(1) << static_cast<uint64_t>(vkKey) % 64;	
	mKeysDownMask[keyEntryIndex]= isDown ?	mKeysDownMask[keyEntryIndex] | keyBitMask : 
											mKeysDownMask[keyEntryIndex] & ~keyBitMask;
}

#else

bool CmdInput::IsKeyDown(ImGuiKey key)const
{
	if (key >= ImGuiKey_COUNT || key < ImGuiKey_NamedKey_BEGIN) {
		return false;
	}

	const uint64_t relateIndex = uint64_t(key) - ImGuiKey_NamedKey_BEGIN;
	const uint64_t keyEntryIndex = static_cast<uint64_t>(relateIndex) / 64;

	if (keyEntryIndex >= 512 / 64) {
		return false;
	}

	return (mKeysDownMask[keyEntryIndex] & (static_cast<uint64_t>(1) << (relateIndex % 64))) != 0;
}

void CmdInput::SetKeyDown(ImGuiKey key, bool isDown)
{
	if (key >= ImGuiKey_COUNT || key < ImGuiKey_NamedKey_BEGIN) {
		return;
	}

	const uint64_t relateIndex = uint64_t(key) - ImGuiKey_NamedKey_BEGIN;
	const uint64_t keyEntryIndex = static_cast<uint64_t>(relateIndex) / 64;

	if (keyEntryIndex >= 512 / 64) {
		return;
	}

	const uint64_t keyBitMask = static_cast<uint64_t>(1) << static_cast<uint64_t>(relateIndex) % 64;
	mKeysDownMask[keyEntryIndex]= isDown ? mKeysDownMask[keyEntryIndex] | keyBitMask :
										   mKeysDownMask[keyEntryIndex] & ~keyBitMask;
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

#endif

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
