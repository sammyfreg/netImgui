#include "NetImgui_CmdPackets.h"

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

bool CmdInput::IsKeyDown( CmdInput::NetImguiKeys netimguiKey) const
{
	uint32_t valIndex	= netimguiKey/64;
	uint64_t valMask	= 0x0000000000000001ull << (netimguiKey%64);
	return mInputDownMask[valIndex] & valMask;
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
