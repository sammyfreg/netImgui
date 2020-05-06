namespace NetImgui { namespace Internal
{

void CmdDrawFrame::ToPointers()
{
	if( !mpVertices.IsPointer() )
	{
		mpVertices.ToPointer();
		mpIndices.ToPointer();
		mpDraws.ToPointer();
	}
}

void CmdDrawFrame::ToOffsets()
{
	if( !mpVertices.IsOffset() )
	{
		mpVertices.ToOffset();
		mpIndices.ToOffset();
		mpDraws.ToOffset();
	}
}

bool CmdInput::IsKeyDown(eVirtualKeys vkKey)const
{
	return (KeysDownMask[vkKey/64] & (uint64_t(1)<<(vkKey%64))) != 0;
}

}} // namespace NetImgui::Internal
