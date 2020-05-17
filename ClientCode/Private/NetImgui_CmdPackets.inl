namespace NetImgui { namespace Internal
{
//SF TODO: Make Offset/Pointer test safer
void CmdDrawFrame::ToPointers()
{
	if( !mpIndices.IsPointer() ) //Safer to test the first element after CmdHeader
	{
		mpVertices.ToPointer();
		mpIndices.ToPointer();
		mpDraws.ToPointer();
	}
}

void CmdDrawFrame::ToOffsets()
{
	if( !mpIndices.IsOffset() ) //Safer to test the first element after CmdHeader
	{
		mpVertices.ToOffset();
		mpIndices.ToOffset();
		mpDraws.ToOffset();
	}
}

bool CmdInput::IsKeyDown(eVirtualKeys vkKey)const
{
	return (mKeysDownMask[vkKey/64] & (uint64_t(1)<<(vkKey%64))) != 0;
}

}} // namespace NetImgui::Internal
