#include <imgui.h>
#include "RemoteImgui_ImguiFrame.h"


namespace RmtImgui
{

template <typename IntType>
IntType RoundUp(IntType Value, unsigned int Round)
{
	return ((Value + Round -1) / Round) * Round;
}

inline void RemoteImgui_ExtractVertices(ImguiVert* pOutVertices, const ImDrawList* pCmdList)
{
	for(int i(0), count(pCmdList->VtxBuffer.size()); i<count; ++i)
	{
		const auto& Vtx = pCmdList->VtxBuffer[i];
		pOutVertices[i].mColor	= Vtx.col;
		pOutVertices[i].mUV[0]	= (uint16_t)((Vtx.uv.x - float(ImguiVert::kUvRange_Min)) * 0x10000 / (ImguiVert::kUvRange_Max - ImguiVert::kUvRange_Min));
		pOutVertices[i].mUV[1]	= (uint16_t)((Vtx.uv.y - float(ImguiVert::kUvRange_Min)) * 0x10000 / (ImguiVert::kUvRange_Max - ImguiVert::kUvRange_Min));
		pOutVertices[i].mPos[0]	= (uint16_t)((Vtx.pos.x - float(ImguiVert::kPosRange_Min)) * 0x10000 / (ImguiVert::kPosRange_Max - ImguiVert::kPosRange_Min));
		pOutVertices[i].mPos[1]	= (uint16_t)((Vtx.pos.y - float(ImguiVert::kPosRange_Min)) * 0x10000 / (ImguiVert::kPosRange_Max - ImguiVert::kPosRange_Min));
	}
}

inline void RemoteImgui_ExtractIndices(uint8_t* pOutIndices, const ImDrawList* pCmdList)
{
	bool is16Bit			= pCmdList->VtxBuffer.size() <= 0xFFFF;
	uint32_t IndexSize		= is16Bit ? 2 : 4;
	uint32_t IndexCount		= pCmdList->IdxBuffer.size();
	// No conversion needed
	if( IndexSize == sizeof(ImDrawIdx) )
	{
		memcpy(pOutIndices, &pCmdList->IdxBuffer.front(), IndexCount*IndexSize); 
	}
	// From 32bits to 16bits
	else if(is16Bit)
	{
	 	for(uint32_t i(0); i < IndexCount; ++i)
	 		reinterpret_cast<uint16_t*>(pOutIndices)[i] = (uint16_t)pCmdList->IdxBuffer[i];
	}
	// From 16bits to 32bits
	else
	{
		for(uint32_t	i(0); i < IndexCount; ++i)
	 		reinterpret_cast<uint32_t*>(pOutIndices)[i] = (uint32_t)pCmdList->IdxBuffer[i];
	}
}

void ImguiFrame::UpdatePointer()
{
	if( mOffsetVertice < (size_t)this )
	{
		mpVertices	= reinterpret_cast<ImguiVert*>((uint8_t*)this + mOffsetVertice);
		mpIndices	= reinterpret_cast<uint8_t*>((uint8_t*)this + mOffsetIndice);
		mpDraws		= reinterpret_cast<ImguiDraw*>((uint8_t*)this + mOffsetDraw);
	}		
}

ImguiFrame* ImguiFrame::Create(const ImDrawData* pDearImguiData)
{
	//-----------------------------------------------------------------------------------------
	// Fetch Header informations (data size)
	//-----------------------------------------------------------------------------------------
	ImguiFrame Header;
	Header.mVerticeCount			= pDearImguiData->TotalVtxCount;
	Header.mDrawCount				= 0;
	Header.mIndiceSize				= 0;
	for(int n = 0; n < pDearImguiData->CmdListsCount; n++)
	{
		const ImDrawList* pCmdList	= pDearImguiData->CmdLists[n];
		bool is16Bit				= pCmdList->VtxBuffer.size() <= 0xFFFF;
		Header.mIndiceSize			+= RoundUp(pCmdList->IdxBuffer.size() * (is16Bit ? 2 : 4), 4);
		Header.mDrawCount			+= pCmdList->CmdBuffer.size();	// Maximum possible, can be less since some are for callbacks
	}		
		
	Header.mDataSize		= 0;
	Header.mOffsetVertice	= Header.mDataSize	+= RoundUp<uint32_t>(sizeof(ImguiFrame), 8);
	Header.mOffsetIndice	= Header.mDataSize	+= RoundUp<uint32_t>(sizeof(ImguiVert)*Header.mVerticeCount, 8);
	Header.mOffsetDraw		= Header.mDataSize	+= RoundUp<uint32_t>(Header.mIndiceSize, 8);
	Header.mDataSize		= RoundUp<uint32_t>(Header.mDataSize + sizeof(ImguiDraw)*Header.mDrawCount, 8);
		
	uint8_t* pRawData		= new uint8_t[Header.mDataSize];
	ImguiFrame* pFrame		= reinterpret_cast<ImguiFrame*>(&pRawData[0]);
	ImguiVert* pVertex		= reinterpret_cast<ImguiVert*>(&pRawData[Header.mOffsetVertice]);
	uint8_t* pIndice		= reinterpret_cast<uint8_t*>(&pRawData[Header.mOffsetIndice]);
	ImguiDraw* pDraw		= reinterpret_cast<ImguiDraw*>(&pRawData[Header.mOffsetDraw]);
		
	//-----------------------------------------------------------------------------------------
	// Copy draw data (vertices, indices, drawcall info, ...)
	//-----------------------------------------------------------------------------------------
	uint32_t VertexIndex(0);
	uint32_t IndiceByteOffset(0);
	uint32_t DrawIndex(0);
	*pFrame = Header;
	for(int n = 0; n < pDearImguiData->CmdListsCount; n++)
	{
		const ImDrawList* pCmdList = pDearImguiData->CmdLists[n];
		bool is16Bit			= pCmdList->VtxBuffer.size() <= 0xFFFF;
		RemoteImgui_ExtractVertices(&pVertex[VertexIndex], pCmdList);
		RemoteImgui_ExtractIndices(&pIndice[IndiceByteOffset], pCmdList);
		for(int cmd_i = 0; cmd_i < pCmdList->CmdBuffer.size(); cmd_i++)
		{
			const ImDrawCmd* pCmd	= &pCmdList->CmdBuffer[cmd_i];				
			if( pCmd->UserCallback == nullptr )
			{					
				pDraw[DrawIndex].mIdxOffset		= IndiceByteOffset;
				pDraw[DrawIndex].mVtxOffset		= VertexIndex;
				pDraw[DrawIndex].mTextureId		= 0; //pCmd->TextureId; //SF TODO
				pDraw[DrawIndex].mIdxCount		= pCmd->ElemCount;
				pDraw[DrawIndex].mIndexSize		= is16Bit ? 2 : 4;
				pDraw[DrawIndex].mClipRect[0]	= pCmd->ClipRect.x;
				pDraw[DrawIndex].mClipRect[1]	= pCmd->ClipRect.y;
				pDraw[DrawIndex].mClipRect[2]	= pCmd->ClipRect.z;
				pDraw[DrawIndex].mClipRect[3]	= pCmd->ClipRect.w;
				IndiceByteOffset				+= pCmd->ElemCount * pDraw[DrawIndex].mIndexSize;
				DrawIndex						+= 1;
			}
		}
		IndiceByteOffset	= RoundUp(IndiceByteOffset, 4);
		VertexIndex			+= pCmdList->VtxBuffer.size();
	}	
	pFrame->mDrawCount		= DrawIndex;	// Not all DrawCmd generate a draw, update value to actual value
	return pFrame;
}

} // namespace RmtImgui