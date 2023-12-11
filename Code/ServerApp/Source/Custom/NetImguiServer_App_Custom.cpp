//=================================================================================================
// This file is meant to be edited by Library user, to add extra functionalities to 
// the NetImguiServer application. It allows to isolate custom user code without interferring
// with NetImgui code updates.
// 
// For now, it is used to handle user defined custom texture formats.
//=================================================================================================

#include "NetImguiServer_App.h"
#include "NetImguiServer_RemoteClient.h"

// Enable handling of a Custom Texture Format samples. 
// Only meant as an example, users are free to replace it with their own handling of
// custom texture formats. Look for this define for implementation details.
#define TEXTURE_CUSTOM_SAMPLE 1

#if TEXTURE_CUSTOM_SAMPLE

//=================================================================================================
// This is our Custom texture data format, it must match when the NetImgui Client code send
// It can be customized to anything needed, as long as Client/Server use the same content
// For this sample, we rely on the struct size and a starting stamp to identify the custom data
struct customTextureData1{ 
	static constexpr uint64_t kStamp = 0x0123456789000001;
	uint64_t m_Stamp		= kStamp;
	uint32_t m_ColorStart	= 0; 
	uint32_t m_ColorEnd		= 0; 
};

struct customTextureData2{
	static constexpr uint64_t kStamp = 0x0123456789000002;
	uint64_t m_Stamp		= kStamp;
	uint64_t m_TimeUS		= 0;
};
//=================================================================================================
#endif

namespace NetImguiServer { namespace App
{

//=================================================================================================
// This contains a sample of letting User create their own texture format, handling it on both
// the NetImgui Client and the NetImgui Server side
// 
// User are entirely free to manage custom texture content as they wish as long as it generates
// a valid 'ServerTexture.mpHAL_Texture' GPU texture object for the Imgui Backend
// 
// Sample was kept simple by reusing HAL_CreateTexture / HAL_DestroyTexture when updating
// the texture, but User is free to create valid texture without these functions
// 
//-------------------------------------------------------------------------------------------------
// Return true if the command was handled
//=================================================================================================
bool CreateTexture_Custom( ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize )
{
	IM_UNUSED(serverTexture);
	IM_UNUSED(cmdTexture);
	IM_UNUSED(customDataSize);
#if TEXTURE_CUSTOM_SAMPLE
	auto eTexFmt = static_cast<NetImgui::eTexFormat>(cmdTexture.mFormat);
	if( eTexFmt == NetImgui::eTexFormat::kTexFmtCustom ){
		
		// Process Custom Texture Type 1
		// This sample custom texture interpolate between 2 colors over the x axis
		const customTextureData1* pCustomData1 = reinterpret_cast<const customTextureData1*>(cmdTexture.mpTextureData.Get());
		if( customDataSize == sizeof(customTextureData1) && pCustomData1->m_Stamp == customTextureData1::kStamp )
		{
			ImColor colorStart			= pCustomData1->m_ColorStart;
			ImColor colorEnd			= pCustomData1->m_ColorEnd;
			uint32_t* pTempData			= static_cast<uint32_t*>(malloc(cmdTexture.mWidth*cmdTexture.mHeight*sizeof(uint32_t)));
			for(uint32_t x=0; x<cmdTexture.mWidth; ++x){
				float ratio				= static_cast<float>(x) / static_cast<float>(cmdTexture.mWidth);
				ImColor colorCurrent	= ImColor(	colorStart.Value.x  * ratio + colorEnd.Value.x * (1.f - ratio),
													colorStart.Value.y  * ratio + colorEnd.Value.y * (1.f - ratio),
													colorStart.Value.z  * ratio + colorEnd.Value.z * (1.f - ratio),
													colorStart.Value.w  * ratio + colorEnd.Value.w * (1.f - ratio));
				for(uint32_t y=0; y<cmdTexture.mHeight; ++y){
					pTempData[y*cmdTexture.mWidth+x] = colorCurrent;
				}
			}
			NetImguiServer::App::HAL_CreateTexture(cmdTexture.mWidth, cmdTexture.mHeight, NetImgui::eTexFormat::kTexFmtRGBA8, reinterpret_cast<uint8_t*>(pTempData), serverTexture);
			free(pTempData);

			// Assign a custom texture information to the Server Texture
			// Here, it's just a stamp, but could be some user specific data, like allocated memory for extra info
			serverTexture.mCustomData = customTextureData1::kStamp;
			return true;
		}
	
		// Process Custom Texture Type 2
		// This sample custom texture display a moving sin wave
		const customTextureData2* pCustomData2 = reinterpret_cast<const customTextureData2*>(cmdTexture.mpTextureData.Get());
		if( customDataSize == sizeof(customTextureData2) && pCustomData2->m_Stamp == customTextureData2::kStamp )
		{
			uint32_t* pTempData	= static_cast<uint32_t*>(malloc(cmdTexture.mWidth*cmdTexture.mHeight*sizeof(uint32_t)));
			for(uint32_t x=0; pTempData && x<cmdTexture.mWidth; ++x)
			{
				constexpr uint64_t kLoopTimeUS	= 1000 * 1000 * 3; // 1 Loop per 10 seconds
				float ratioX					= static_cast<float>(x) / static_cast<float>(cmdTexture.mWidth);
				uint64_t timeRatio				= pCustomData2->m_TimeUS % kLoopTimeUS; 
				float sinRatio					= (static_cast<float>(timeRatio) / static_cast<float>(kLoopTimeUS) + ratioX) * 3.1415f * 2.0f;
				float sinVal					= static_cast<float>(sin(sinRatio));
				for(uint32_t y=0; y<cmdTexture.mHeight; ++y){
					float ratioY = static_cast<float>(y) / static_cast<float>(cmdTexture.mHeight) * 2.f - 1.f;
					float dist = sinVal - ratioY;
					float r = dist > 0.f ? 1.f - dist/2.f : 0.f;
					float b = dist < 0.f ? 1.f + dist/2.f : 0.f;
					ImColor colorCurrent	= ImColor(	r, 0.f, b, 1.f);
					pTempData[y*cmdTexture.mWidth+x] = colorCurrent;
				}
			}
			NetImguiServer::App::HAL_CreateTexture(cmdTexture.mWidth, cmdTexture.mHeight, NetImgui::eTexFormat::kTexFmtRGBA8, reinterpret_cast<uint8_t*>(pTempData), serverTexture);
			free(pTempData);
			// Assign a custom texture information to the Server Texture
			// Here, it's just a stamp, but could be some user specific data, like allocated memory for extra info
			serverTexture.mCustomData = customTextureData2::kStamp;
			return true;
		}
	}
#endif
	return false;
}

//=================================================================================================
// Return true if the command was handled
bool DestroyTexture_Custom( ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize )
//=================================================================================================
{
	IM_UNUSED(serverTexture);
	IM_UNUSED(cmdTexture);
	IM_UNUSED(customDataSize);

#if TEXTURE_CUSTOM_SAMPLE
	if( serverTexture.mpHAL_Texture && serverTexture.mFormat == NetImgui::eTexFormat::kTexFmtCustom ){
		if( serverTexture.mCustomData == customTextureData1::kStamp || 
			serverTexture.mCustomData == customTextureData2::kStamp )
		{
			// Could decide to only update the texture instead of recreating it
			// But this sample just delete it (like default behavior)
			if ( serverTexture.mFormat == cmdTexture.mFormat &&
				 serverTexture.mSize[0] == cmdTexture.mWidth &&
				 serverTexture.mSize[1] == cmdTexture.mHeight )
			{
				NetImguiServer::App::EnqueueHALTextureDestroy(serverTexture);
				return true;
			}
			// Texture format changed or requested to be deleted
			else
			{		
				NetImguiServer::App::EnqueueHALTextureDestroy(serverTexture);
				return true;
			}
		}
	}
#endif
	return false;
}

} } //namespace NetImguiServer { namespace App
