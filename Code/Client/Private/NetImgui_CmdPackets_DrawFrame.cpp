#include "NetImgui_Shared.h"

#if NETIMGUI_ENABLED
#include "NetImgui_WarningDisable.h"
#include "NetImgui_CmdPackets.h"

namespace NetImgui { namespace Internal
{

template <typename IntType>
IntType RoundUp(IntType Value, IntType Round)
{
	return ((Value + Round -1) / Round) * Round;
}

template <typename TType>
inline void SetAndIncreaseDataPointer(OffsetPointer<TType>& dataPointer, uint32_t dataSize, uint8_t*& pDataOutput)
{
	dataPointer.SetPtr(reinterpret_cast<TType*>(pDataOutput));
	const uint32_t dataSizeAligned	= RoundUp(dataSize, 8u);
	const uint32_t dataSizePadding	= dataSizeAligned - dataSize;
	if( dataSizePadding >= 7 )	pDataOutput[dataSize+0] = 0;
	if( dataSizePadding >= 6 )	pDataOutput[dataSize+1] = 0;
	if( dataSizePadding >= 5 )	pDataOutput[dataSize+2] = 0;
	if( dataSizePadding >= 4 )	pDataOutput[dataSize+3] = 0;
	if( dataSizePadding >= 3 )	pDataOutput[dataSize+4] = 0;
	if( dataSizePadding >= 2 )	pDataOutput[dataSize+5] = 0;
	if( dataSizePadding >= 1 )	pDataOutput[dataSize+6] = 0;
	pDataOutput += dataSizeAligned;
}

//=================================================================================================
// 
//=================================================================================================
inline void ImGui_ExtractIndices(const ImDrawList& cmdList, ImguiDrawGroup& drawGroupOut, uint8_t*& pDataOutput)
{
	bool is16Bit					= sizeof(ImDrawIdx) == 2 || cmdList.VtxBuffer.size() <= 0xFFFF;	// When Dear Imgui is compiled with ImDrawIdx = uint16, we know for certain that there won't be any drawcall with index > 65k, even if Vertex buffer is bigger than 65k.
	drawGroupOut.mBytePerIndex		= is16Bit ? 2 : 4;
	drawGroupOut.mIndiceCount		= static_cast<uint32_t>(cmdList.IdxBuffer.size());

	uint32_t sizeNeeded				= drawGroupOut.mIndiceCount*drawGroupOut.mBytePerIndex;
	SetAndIncreaseDataPointer(drawGroupOut.mpIndices, sizeNeeded, pDataOutput);

	// No conversion needed, straight copy
	if( drawGroupOut.mBytePerIndex == sizeof(ImDrawIdx) )
	{
		memcpy(drawGroupOut.mpIndices.Get(), &cmdList.IdxBuffer.front(), sizeNeeded);
	}
	// From 32bits to 16bits
	else if(is16Bit)
	{
	 	for(int i(0); i < static_cast<int>(drawGroupOut.mIndiceCount); ++i)
	 		reinterpret_cast<uint16_t*>(drawGroupOut.mpIndices.Get())[i] = static_cast<uint16_t>(cmdList.IdxBuffer[i]);
	}
	// From 16bits to 32bits
	else
	{
		for(int i(0); i < static_cast<int>(drawGroupOut.mIndiceCount); ++i)
	 		reinterpret_cast<uint32_t*>(drawGroupOut.mpIndices.Get())[i] = static_cast<uint32_t>(cmdList.IdxBuffer[i]);
	}
}

//=================================================================================================
// 
//=================================================================================================
inline void ImGui_ExtractVertices(const ImDrawList& cmdList, ImguiDrawGroup& drawGroupOut, uint8_t*& pDataOutput)
{
	drawGroupOut.mVerticeCount	= static_cast<uint32_t>(cmdList.VtxBuffer.size());
	SetAndIncreaseDataPointer(drawGroupOut.mpVertices, drawGroupOut.mVerticeCount*sizeof(ImguiVert), pDataOutput);
	ImguiVert* pVertices = drawGroupOut.mpVertices.Get();
	for(int i(0); i<static_cast<int>(drawGroupOut.mVerticeCount); ++i)
	{
		const auto& Vtx			= cmdList.VtxBuffer[i];
		pVertices[i].mColor		= Vtx.col;
		pVertices[i].mUV[0]		= static_cast<uint16_t>((Vtx.uv.x	- static_cast<float>(ImguiVert::kUvRange_Min)) * 0xFFFF / (ImguiVert::kUvRange_Max - ImguiVert::kUvRange_Min));
		pVertices[i].mUV[1]		= static_cast<uint16_t>((Vtx.uv.y	- static_cast<float>(ImguiVert::kUvRange_Min)) * 0xFFFF / (ImguiVert::kUvRange_Max - ImguiVert::kUvRange_Min));
		pVertices[i].mPos[0]	= static_cast<uint16_t>((Vtx.pos.x	- static_cast<float>(ImguiVert::kPosRange_Min)) * 0xFFFF / (ImguiVert::kPosRange_Max - ImguiVert::kPosRange_Min));
		pVertices[i].mPos[1]	= static_cast<uint16_t>((Vtx.pos.y	- static_cast<float>(ImguiVert::kPosRange_Min)) * 0xFFFF / (ImguiVert::kPosRange_Max - ImguiVert::kPosRange_Min));
	}
}

//=================================================================================================
// 
//=================================================================================================
inline void ImGui_ExtractDraws(const ImDrawList& cmdList, ImguiDrawGroup& drawGroupOut, uint8_t*& pDataOutput)
{
	int maxDrawCount		= static_cast<int>(cmdList.CmdBuffer.size());
	uint32_t drawCount		= 0;
	ImguiDraw* pOutDraws	= reinterpret_cast<ImguiDraw*>(pDataOutput);
	for(int cmd_i = 0; cmd_i < maxDrawCount; ++cmd_i)
	{
		const ImDrawCmd* pCmd = &cmdList.CmdBuffer[cmd_i];
		if( pCmd->UserCallback == nullptr )
		{
		#if IMGUI_VERSION_NUM >= 17100
			pOutDraws[drawCount].mVtxOffset		= pCmd->VtxOffset;
			pOutDraws[drawCount].mIdxOffset		= pCmd->IdxOffset;
		#else
			pOutDraws[drawCount].mVtxOffset		= 0;
			pOutDraws[drawCount].mIdxOffset		= 0;
		#endif
			
			pOutDraws[drawCount].mTextureId		= TextureCastHelper(pCmd->TextureId);
			pOutDraws[drawCount].mIdxCount		= pCmd->ElemCount;
			pOutDraws[drawCount].mClipRect[0]	= pCmd->ClipRect.x;
			pOutDraws[drawCount].mClipRect[1]	= pCmd->ClipRect.y;
			pOutDraws[drawCount].mClipRect[2]	= pCmd->ClipRect.z;
			pOutDraws[drawCount].mClipRect[3]	= pCmd->ClipRect.w;
			++drawCount;
		}
	}
	drawGroupOut.mDrawCount = drawCount;
	SetAndIncreaseDataPointer(drawGroupOut.mpDraws, drawGroupOut.mDrawCount*sizeof(ImguiDraw), pDataOutput);
}

//=================================================================================================
// 
//=================================================================================================
void CompressData(const uint64_t* pDataPrev, size_t dataSizePrev, const uint64_t* pDataNew, size_t dataSizeNew, uint64_t*& pCommandMemoryInOut)
{
	//assert(dataSizePrev % sizeof(uint64_t) == 0); //SF TODO make sure everything is 64bits aligned, preventing bad data
	//assert(dataSizeNew % sizeof(uint64_t) == 0);
	const uint32_t elemCountPrev	= static_cast<uint32_t>(dataSizePrev / sizeof(uint64_t));
	const uint32_t elemCountNew		= static_cast<uint32_t>(dataSizeNew / sizeof(uint64_t));
	const uint32_t elemCount		= std::min(elemCountPrev, elemCountNew);
	uint32_t n						= 0;
	
	while(n < elemCount)
	{
		uint32_t* pBlockInfo = reinterpret_cast<uint32_t*>(pCommandMemoryInOut++); // Add a new block info to output

		// Find number of elements with same value as last frame
		uint32_t startN = n;
		while( n < elemCount && pDataPrev[n] == pDataNew[n] )
			++n;
		pBlockInfo[0] = n - startN;

		// Find number of elements with different value as last frame, and save new value
		while (n < elemCount && pDataPrev[n] != pDataNew[n]) {
			*pCommandMemoryInOut = pDataNew[n++];
			++pCommandMemoryInOut;
		}
		pBlockInfo[1]	= static_cast<uint32_t>(pCommandMemoryInOut - reinterpret_cast<uint64_t*>(pBlockInfo)) - 1;
	}

	// New frame has more element than previous frame, add the remaining entries
	if(elemCount < elemCountNew)
	{
		uint32_t* pBlockInfo = reinterpret_cast<uint32_t*>(pCommandMemoryInOut++); // Add a new block info to output
		while (n < elemCountNew) {
			*pCommandMemoryInOut = pDataNew[n++];
			++pCommandMemoryInOut;
		}
		pBlockInfo[0] = 0;
		pBlockInfo[1] = static_cast<uint32_t>(pCommandMemoryInOut - reinterpret_cast<uint64_t*>(pBlockInfo)) - 1;
	}
}

void DecompressData(const uint64_t* pDataPrev, size_t dataSizePrev, const uint64_t* pDataPack, size_t dataUnpackSize, uint64_t*& pCommandMemoryInOut)
{
	//assert(dataSizePrev % sizeof(uint64_t) == 0); //SF TODO make sure everything is 64bits aligned, preventing bad data
	const uint32_t elemCountPrev	= static_cast<uint32_t>(dataSizePrev / sizeof(uint64_t));
	const uint32_t elemCountUnpack	= static_cast<uint32_t>(dataUnpackSize / sizeof(uint64_t));
	const uint32_t elemCountCopy	= std::min(elemCountPrev, elemCountUnpack);
	uint64_t* pCommandMemoryEnd		= &pCommandMemoryInOut[elemCountUnpack];
	memcpy(pCommandMemoryInOut, pDataPrev, elemCountCopy * sizeof(uint64_t));
	while(pCommandMemoryInOut < pCommandMemoryEnd)
	{
		const uint32_t* pBlockInfo	= reinterpret_cast<const uint32_t*>(pDataPack++); // Add a new block info to output
		pCommandMemoryInOut			+= pBlockInfo[0];
		memcpy(pCommandMemoryInOut, pDataPack, pBlockInfo[1] * sizeof(uint64_t));
		pCommandMemoryInOut			+= pBlockInfo[1];
		pDataPack					+= pBlockInfo[1];
	}
}

//=================================================================================================
// Take a regular NetImgui DrawFrame command and create a new compressed command
// It uses a basic delta compression method that works really well with Imgui data
//	- Most of the drawing data do not change between 2 frames
//  - Even if 1 window content changes, the others windows probably won't be changing at all
//  - This means that for each window, we can only send the data that changed
//  - This requires little cpu usage and generate good results
//    - In 'SampleBasic' with 3 windows open (Main Window, ImGui Demo, ImGui Metric) at 30fps
//		- Compression Off: 1650KB/sec of transfert
//      - Compression On : 12KB/sec of transfert (130x less data)
//=================================================================================================
CmdDrawFrame* CompressCmdDrawFrame(const CmdDrawFrame* pDrawFramePrev, const CmdDrawFrame* pDrawFrameNew)
{
	//-----------------------------------------------------------------------------------------
	// Allocate memory for the new compressed command
	//-----------------------------------------------------------------------------------------
	// Allocate memory for worst case scenario (no compression possible)
	// New DrawFrame size + 2 'compression block info' per data stream
	uint32_t neededDataSize			= static_cast<uint32_t>(RoundUp(pDrawFrameNew->mHeader.mSize + pDrawFrameNew->mDrawGroupCount * 6 * sizeof(uint64_t), sizeof(uint64_t)));
	CmdDrawFrame* pDrawFramePacked	= netImguiSizedNew<CmdDrawFrame>(neededDataSize);
	uint64_t* pDataOutput			= reinterpret_cast<uint64_t*>(&pDrawFramePacked[1]);

	memcpy(pDrawFramePacked, pDrawFrameNew, sizeof(CmdDrawFrame));
	pDrawFramePacked->mCompressed	= true;
	pDrawFramePacked->mpDrawGroups.SetPtr(reinterpret_cast<ImguiDrawGroup*>(pDataOutput));
	pDataOutput						+= (pDrawFramePacked->mDrawGroupCount * sizeof(ImguiDrawGroup)) / sizeof(uint64_t);

	//-----------------------------------------------------------------------------------------
	// Copy draw data (vertices, indices, drawcall info, ...)
	//-----------------------------------------------------------------------------------------
	const uint32_t groupCountPrev = pDrawFramePrev->mDrawGroupCount;
	for(uint32_t n = 0; n < pDrawFramePacked->mDrawGroupCount; n++)
	{
		// Look for the same drawgroup in previous frame
		// Can usually avoid a search by checking same index in previous frame (drawgroup ordering shouldn't change often)		
		const ImguiDrawGroup& drawGroupNew	= pDrawFrameNew->mpDrawGroups[n];
		ImguiDrawGroup& drawGroup			= pDrawFramePacked->mpDrawGroups[n];
		drawGroup							= drawGroupNew;
		drawGroup.mDrawGroupIdxPrev			= (n < groupCountPrev && drawGroup.mGroupID == pDrawFramePrev->mpDrawGroups[n].mGroupID) ? n : ImguiDrawGroup::kInvalidDrawGroup;
		for(uint32_t j(0); j<groupCountPrev && drawGroup.mDrawGroupIdxPrev == ImguiDrawGroup::kInvalidDrawGroup; ++j){
			drawGroup.mDrawGroupIdxPrev = (drawGroup.mGroupID == pDrawFramePrev->mpDrawGroups[j].mGroupID) ? j : ImguiDrawGroup::kInvalidDrawGroup;
		}

		// Delta compress the 3 data streams
		const uint64_t *pVerticePrev(nullptr), *pIndicePrev(nullptr), *pDrawsPrev(nullptr);
		size_t verticeSizePrev(0), indiceSizePrev(0), drawSizePrev(0);
		if (drawGroup.mDrawGroupIdxPrev < pDrawFramePrev->mDrawGroupCount) {
			const ImguiDrawGroup& drawGroupPrev = pDrawFramePrev->mpDrawGroups[drawGroup.mDrawGroupIdxPrev];
			pVerticePrev						= reinterpret_cast<const uint64_t*>(drawGroupPrev.mpVertices.Get());
			pIndicePrev							= reinterpret_cast<const uint64_t*>(drawGroupPrev.mpIndices.Get());
			pDrawsPrev							= reinterpret_cast<const uint64_t*>(drawGroupPrev.mpDraws.Get());
			verticeSizePrev						= drawGroupPrev.mVerticeCount * sizeof(ImguiVert);
			indiceSizePrev						= drawGroupPrev.mIndiceCount*drawGroupPrev.mBytePerIndex;
			drawSizePrev						= drawGroupPrev.mDrawCount*sizeof(ImguiDraw);
		}

		drawGroup.mpIndices.SetPtr(reinterpret_cast<uint8_t*>(pDataOutput));
		CompressData( pIndicePrev,	indiceSizePrev,		reinterpret_cast<const uint64_t*>(drawGroupNew.mpIndices.Get()),	drawGroupNew.mIndiceCount*drawGroupNew.mBytePerIndex,	pDataOutput);

		drawGroup.mpVertices.SetPtr(reinterpret_cast<ImguiVert*>(pDataOutput));
		CompressData( pVerticePrev, verticeSizePrev,	reinterpret_cast<const uint64_t*>(drawGroupNew.mpVertices.Get()),	drawGroupNew.mVerticeCount * sizeof(ImguiVert),			pDataOutput);

		drawGroup.mpDraws.SetPtr(reinterpret_cast<ImguiDraw*>(pDataOutput));
		CompressData( pDrawsPrev,	drawSizePrev,		reinterpret_cast<const uint64_t*>(drawGroupNew.mpDraws.Get()),		drawGroupNew.mDrawCount*sizeof(ImguiDraw),				pDataOutput);
	}

	// Adjust data transfert amount to memory that has been actually needed
	pDrawFramePacked->mHeader.mSize = static_cast<uint32_t>((pDataOutput - reinterpret_cast<uint64_t*>(pDrawFramePacked)))*static_cast<uint32_t>(sizeof(uint64_t));
	return pDrawFramePacked;
}

//=================================================================================================
// 
//=================================================================================================
CmdDrawFrame* DecompressCmdDrawFrame(const CmdDrawFrame* pDrawFramePrev, const CmdDrawFrame* pDrawFramePacked)
{
	//-----------------------------------------------------------------------------------------
	// Allocate memory for the new uncompressed compressed command
	//-----------------------------------------------------------------------------------------
	CmdDrawFrame* pDrawFrameNew		= netImguiSizedNew<CmdDrawFrame>(pDrawFramePacked->mUncompressedSize);
	uint64_t* pDataOutput			= reinterpret_cast<uint64_t*>(&pDrawFrameNew[1]);

	memcpy(pDrawFrameNew, pDrawFramePacked, sizeof(CmdDrawFrame));
	pDrawFrameNew->mpDrawGroups.SetPtr(reinterpret_cast<ImguiDrawGroup*>(pDataOutput));
	pDrawFrameNew->mCompressed		= false;
	pDataOutput						+= (pDrawFrameNew->mDrawGroupCount * sizeof(ImguiDrawGroup)) / sizeof(uint64_t);
	
	for(uint32_t n = 0; n < pDrawFrameNew->mDrawGroupCount; n++)
	{
		const ImguiDrawGroup& drawGroupPack	= pDrawFramePacked->mpDrawGroups[n];
		ImguiDrawGroup& drawGroup			= pDrawFrameNew->mpDrawGroups[n];
		drawGroup							= drawGroupPack;

		// Uncompress the 3 data streams
		const uint64_t* pVerticePrev			= nullptr;
		const uint64_t* pIndicePrev				= nullptr;
		const uint64_t* pDrawsPrev				= nullptr;
		size_t verticeSizePrev(0), indiceSizePrev(0), drawSizePrev(0);
		if (drawGroup.mDrawGroupIdxPrev < pDrawFramePrev->mDrawGroupCount) {
			const ImguiDrawGroup& drawGroupPrev = pDrawFramePrev->mpDrawGroups[drawGroup.mDrawGroupIdxPrev];
			pVerticePrev						= reinterpret_cast<const uint64_t*>(drawGroupPrev.mpVertices.Get());
			pIndicePrev							= reinterpret_cast<const uint64_t*>(drawGroupPrev.mpIndices.Get());
			pDrawsPrev							= reinterpret_cast<const uint64_t*>(drawGroupPrev.mpDraws.Get());
			verticeSizePrev						= drawGroupPrev.mVerticeCount * sizeof(ImguiVert);
			indiceSizePrev						= drawGroupPrev.mIndiceCount*drawGroupPrev.mBytePerIndex;
			drawSizePrev						= drawGroupPrev.mDrawCount*sizeof(ImguiDraw);
		}
		
		drawGroup.mpIndices.SetPtr(reinterpret_cast<uint8_t*>(pDataOutput));
		DecompressData( pIndicePrev,	indiceSizePrev,		reinterpret_cast<const uint64_t*>(drawGroupPack.mpIndices.Get()),	drawGroupPack.mIndiceCount*drawGroupPack.mBytePerIndex,	pDataOutput);

		drawGroup.mpVertices.SetPtr(reinterpret_cast<ImguiVert*>(pDataOutput));
		DecompressData( pVerticePrev,	verticeSizePrev,	reinterpret_cast<const uint64_t*>(drawGroupPack.mpVertices.Get()),	drawGroupPack.mVerticeCount * sizeof(ImguiVert),		pDataOutput);
			
		drawGroup.mpDraws.SetPtr(reinterpret_cast<ImguiDraw*>(pDataOutput));
		DecompressData( pDrawsPrev,		drawSizePrev,		reinterpret_cast<const uint64_t*>(drawGroupPack.mpDraws.Get()),		drawGroupPack.mDrawCount*sizeof(ImguiDraw),				pDataOutput);
	}
	return pDrawFrameNew;
}

//=================================================================================================
// Take a regular Dear Imgui Draw Data, and convert it to a NetImgui DrawFrame Command
// It involves saving each window draw group vertex/indices/draw buffers 
// and packing their data a little bit, to reduce the bandwidth usage
//=================================================================================================
CmdDrawFrame* ConvertToCmdDrawFrame(const ImDrawData* pDearImguiData, ImGuiMouseCursor mouseCursor)
{
	//-----------------------------------------------------------------------------------------
	// Find memory needed for entire DrawFrame Command
	//-----------------------------------------------------------------------------------------
	uint32_t neededDataSize(sizeof(CmdDrawFrame));
	neededDataSize += RoundUp<uint32_t>(static_cast<uint32_t>(pDearImguiData->CmdListsCount) * sizeof(ImguiDrawGroup), 8u);
	for(int n = 0; n < pDearImguiData->CmdListsCount; n++)
	{
		const ImDrawList* pCmdList	= pDearImguiData->CmdLists[n];
		bool is16Bit				= pCmdList->VtxBuffer.size() <= 0xFFFF;
		neededDataSize				+= RoundUp<uint32_t>(static_cast<uint32_t>(pCmdList->VtxBuffer.size()) * sizeof(ImguiVert), 8u);
		neededDataSize				+= RoundUp<uint32_t>(static_cast<uint32_t>(pCmdList->IdxBuffer.size()) * (is16Bit ? 2 : 4), 8u);
		neededDataSize				+= RoundUp<uint32_t>(static_cast<uint32_t>(pCmdList->CmdBuffer.size()) * sizeof(ImguiDraw), 8u);
	}

	//-----------------------------------------------------------------------------------------
	// Allocate Data and init general frame informations
	//-----------------------------------------------------------------------------------------	
	CmdDrawFrame* pDrawFrame		= netImguiSizedNew<CmdDrawFrame>(neededDataSize);
	uint8_t* pDataOutput			= reinterpret_cast<uint8_t*>(&pDrawFrame[1]);
	pDrawFrame->mMouseCursor		= static_cast<uint32_t>(mouseCursor);
	pDrawFrame->mDisplayArea[0]		= pDearImguiData->DisplayPos.x;
	pDrawFrame->mDisplayArea[1]		= pDearImguiData->DisplayPos.y;
	pDrawFrame->mDisplayArea[2]		= pDearImguiData->DisplayPos.x + pDearImguiData->DisplaySize.x;
	pDrawFrame->mDisplayArea[3]		= pDearImguiData->DisplayPos.y + pDearImguiData->DisplaySize.y;
	pDrawFrame->mDrawGroupCount		= static_cast<uint32_t>(pDearImguiData->CmdListsCount);
	SetAndIncreaseDataPointer(pDrawFrame->mpDrawGroups, static_cast<uint32_t>(pDrawFrame->mDrawGroupCount * sizeof(ImguiDrawGroup)), pDataOutput);
	
	//-----------------------------------------------------------------------------------------
	// Copy draw data (vertices, indices, drawcall info, ...)
	//-----------------------------------------------------------------------------------------
	for(uint32_t n = 0; n < pDrawFrame->mDrawGroupCount; n++)
	{
		ImguiDrawGroup& drawGroup	= pDrawFrame->mpDrawGroups[n];
		drawGroup					= ImguiDrawGroup();
		drawGroup.mGroupID			= reinterpret_cast<uint64_t>(pDearImguiData->CmdLists[n]->_OwnerName); // Use the name string pointer as a unique ID (seems to remain the same between frame)
		ImGui_ExtractVertices(*pDearImguiData->CmdLists[n],	drawGroup, pDataOutput);
		ImGui_ExtractIndices(*pDearImguiData->CmdLists[n],	drawGroup, pDataOutput);
		ImGui_ExtractDraws(*pDearImguiData->CmdLists[n],	drawGroup, pDataOutput);
		
		pDrawFrame->mTotalVerticeCount	+= drawGroup.mVerticeCount;
		pDrawFrame->mTotalIndiceCount	+= drawGroup.mIndiceCount;
		pDrawFrame->mTotalDrawCount		+= drawGroup.mDrawCount;
	}
	
	pDrawFrame->mHeader.mSize		= static_cast<uint32_t>(pDataOutput - reinterpret_cast<uint8_t*>(pDrawFrame));
	pDrawFrame->mUncompressedSize	= pDrawFrame->mHeader.mSize;
	return pDrawFrame;
}

}} // namespace NetImgui::Internal

#include "NetImgui_WarningReenable.h"
#endif //#if NETIMGUI_ENABLED
