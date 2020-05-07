#include "NetImGui_DrawFrame.h"
#include "NetImgui_CmdPackets.h"

namespace NetImgui { namespace Internal
{

template <typename IntType>
IntType RoundUp(IntType Value, unsigned int Round)
{
	return ((Value + Round -1) / Round) * Round;
}

inline void ImGui_ExtractVertices(ImguiVert* pOutVertices, const ImDrawList* pCmdList)
{
	for(int i(0), count(pCmdList->VtxBuffer.size()); i<count; ++i)
	{
		const auto& Vtx			= pCmdList->VtxBuffer[i];
		pOutVertices[i].mColor	= Vtx.col;
		pOutVertices[i].mUV[0]	= (uint16_t)((Vtx.uv.x - float(ImguiVert::kUvRange_Min)) * 0x10000 / (ImguiVert::kUvRange_Max - ImguiVert::kUvRange_Min));
		pOutVertices[i].mUV[1]	= (uint16_t)((Vtx.uv.y - float(ImguiVert::kUvRange_Min)) * 0x10000 / (ImguiVert::kUvRange_Max - ImguiVert::kUvRange_Min));
		pOutVertices[i].mPos[0]	= (uint16_t)((Vtx.pos.x - float(ImguiVert::kPosRange_Min)) * 0x10000 / (ImguiVert::kPosRange_Max - ImguiVert::kPosRange_Min));
		pOutVertices[i].mPos[1]	= (uint16_t)((Vtx.pos.y - float(ImguiVert::kPosRange_Min)) * 0x10000 / (ImguiVert::kPosRange_Max - ImguiVert::kPosRange_Min));
	}	
}

inline void ImGui_ExtractIndices(uint8_t* pOutIndices, const ImDrawList* pCmdList)
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

inline void ImGui_ExtractDraws(uint32_t& indiceByteOffset, uint32_t& vertexIndex, uint32_t& drawIndex, ImguiDraw* pOutDraws, const ImDrawList* pCmdList)
{
	const bool is16Bit = pCmdList->VtxBuffer.size() <= 0xFFFF;
	for(int cmd_i = 0; cmd_i < pCmdList->CmdBuffer.size(); cmd_i++)
	{
		const ImDrawCmd* pCmd	= &pCmdList->CmdBuffer[cmd_i];						
		if( pCmd->UserCallback == nullptr )
		{					
			pOutDraws[drawIndex].mIdxOffset		= indiceByteOffset;
			pOutDraws[drawIndex].mVtxOffset		= vertexIndex;
			pOutDraws[drawIndex].mTextureId		= reinterpret_cast<uint64_t>(pCmd->TextureId);
			pOutDraws[drawIndex].mIdxCount		= pCmd->ElemCount;
			pOutDraws[drawIndex].mIndexSize		= is16Bit ? 2 : 4;
			pOutDraws[drawIndex].mClipRect[0]	= pCmd->ClipRect.x;
			pOutDraws[drawIndex].mClipRect[1]	= pCmd->ClipRect.y;
			pOutDraws[drawIndex].mClipRect[2]	= pCmd->ClipRect.z;
			pOutDraws[drawIndex].mClipRect[3]	= pCmd->ClipRect.w;
			indiceByteOffset					+= pCmd->ElemCount * pOutDraws[drawIndex].mIndexSize;
			drawIndex							+= 1;
		}
	}
	indiceByteOffset = RoundUp(indiceByteOffset, 4);
}

CmdDrawFrame* CreateCmdDrawDrame(const ImDrawData* pDearImguiData)
{
	//-----------------------------------------------------------------------------------------
	// Find memory needed for all the data
	//-----------------------------------------------------------------------------------------
	uint32_t indiceSize(0), drawCount(0), dataSize(sizeof(CmdDrawFrame));
	for(int n = 0; n < pDearImguiData->CmdListsCount; n++)
	{
		const ImDrawList* pCmdList	= pDearImguiData->CmdLists[n];
		bool is16Bit				= pCmdList->VtxBuffer.size() <= 0xFFFF;
		indiceSize					+= RoundUp(pCmdList->IdxBuffer.size() * (is16Bit ? 2 : 4), 4);
		drawCount					+= pCmdList->CmdBuffer.size();	// Maximum possible, can be less since some are for callbacks
	}		
			
	uint32_t indiceOffset			= dataSize;	
	dataSize						+= RoundUp<uint32_t>(indiceSize, 8);
	uint32_t verticeOffset			= dataSize;	 
	dataSize						+= RoundUp<uint32_t>(sizeof(ImguiVert)*pDearImguiData->TotalVtxCount, 8);
	uint32_t drawOffset				= dataSize;
	dataSize						+= RoundUp<uint32_t>(sizeof(ImguiDraw)*drawCount, 8);

	//-----------------------------------------------------------------------------------------
	// Allocate Data and init general frame informations
	//-----------------------------------------------------------------------------------------
	uint8_t* pRawData				= CastMalloc<uint8_t>(dataSize);
	CmdDrawFrame* pDrawFrame		= new(pRawData) CmdDrawFrame();	
	pDrawFrame->mVerticeCount		= pDearImguiData->TotalVtxCount;
	pDrawFrame->mIndiceByteSize		= indiceSize;
	pDrawFrame->mDrawCount			= 0;
	pDrawFrame->mpIndices.mPointer	= reinterpret_cast<uint8_t*>(pRawData + indiceOffset);
	pDrawFrame->mpVertices.mPointer	= reinterpret_cast<ImguiVert*>(pRawData + verticeOffset);
	pDrawFrame->mpDraws.mPointer	= reinterpret_cast<ImguiDraw*>(pRawData + drawOffset);
			
	//-----------------------------------------------------------------------------------------
	// Copy draw data (vertices, indices, drawcall info, ...)
	//-----------------------------------------------------------------------------------------
	uint32_t vertexIndex(0), indiceByteOffset(0), drawIndex(0);	
	for(int n = 0; n < pDearImguiData->CmdListsCount; n++)
	{
		const ImDrawList* pCmdList = pDearImguiData->CmdLists[n];
		ImGui_ExtractVertices(&pDrawFrame->mpVertices[vertexIndex], pCmdList);
		ImGui_ExtractIndices(&pDrawFrame->mpIndices[indiceByteOffset], pCmdList);
		ImGui_ExtractDraws(indiceByteOffset, vertexIndex, drawIndex, pDrawFrame->mpDraws.Get(), pCmdList);

		vertexIndex += pCmdList->VtxBuffer.size();		
	}	
	pDrawFrame->mDrawCount		= drawIndex;	// Not all DrawCmd generate a draw, update value to actual value
	pDrawFrame->mHeader.mSize	= dataSize - (drawCount-drawIndex)*sizeof(ImguiDraw);
	pDrawFrame->ToOffsets();
	return pDrawFrame;
}

}} // namespace NetImgui::Internal