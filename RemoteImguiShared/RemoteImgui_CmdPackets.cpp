#include "RemoteImgui_CmdPackets.h"

namespace RmtImgui
{

template <>
void DeserializeCommand(CmdTexture*& pOutCommand, void* pData)
{
	pOutCommand					= reinterpret_cast<CmdTexture*>(pData);
	pOutCommand->mpTextureData	= reinterpret_cast<uint8_t*>((size_t)pOutCommand + sizeof(CmdTexture));
}

template <>
void DeserializeCommand(CmdDrawFrame*& pOutCommand, void* pData)
{
	pOutCommand					= reinterpret_cast<CmdDrawFrame*>(pData);
	pOutCommand->mpFrameData	= reinterpret_cast<ImguiFrame*>((size_t)pOutCommand + sizeof(CmdDrawFrame));
}

} // namespace RmtImgui