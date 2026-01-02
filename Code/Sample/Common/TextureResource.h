#pragma once
//=================================================================================================
// Classes managing the 2 types of textures resource that Samples can use :
//	- Dear ImGui Textures
//	- User Textures
//
// Note: 	It is important to call 'Update()' every frame on the created texture,
//			to handle their destruction when requested
//=================================================================================================
#include <NetImgui_Api.h>
#include "imgui.h"

//=================================================================================================
// Dear ImGui Texture
//-------------------------------------------------------------------------------------------------
// Handle GPU Textures relying on the Dear ImGui Backend to create/update/destroy them and forward
// them to the NetImgui Server. This is mostly done automatically, we just need to provide 
// the format informations and set the pixels data.
//
// Note: Backend only handle RGBA8 format at the moment
//=================================================================================================
class TexResImgui
{
public:
	// Initialise the ImTextureData to be ready for display (pixels will be allocated but unset)
	TexResImgui&	InitPixels(ImTextureFormat Format, int width, int height);
	// Ask the Backend to create the GPU texture
	void 			Create();
	// Get ready to stop using the texture, for destruction on next frame
	void			MarkForDestroy();
	// On pending delete, ask the Backend to destroy the GPU texture and do some cleanup
	void 			Update();
	// Return true when GPU texture can be used for display
	bool 			IsValid() const;
	// Let the Backend know to update a region of GPU texture (after change to pixel data)
	void			MarkUpdated(int x, int y, int w, int h);

	ImTextureRef 	mImTexRef;	// Texture ID to use with Dear ImGui API
	ImTextureData	mImTexData;	// Texture data used by Dear ImGui Backend
};

//=================================================================================================
// User Texture
//-------------------------------------------------------------------------------------------------
// Handle the creation of GPU Textures that the user has to manually manage
// (asking Graphics API to create and destoy them directly)
// For this sample, we're re-using the original DirectX11 code to texture creation
//
// Note: 	Relies on NetImgui texture format specifiers (for local and remote dispaly),
//			but only RGBA8 is supported on local draws
//=================================================================================================
#if NETIMGUI_ENABLED
class TexResUser
{
public:
	// Initialise the format and allocate the pixels memory (pixels are unset)
	TexResUser&		InitPixels(NetImgui::eTexFormat Format, uint16_t width, uint16_t height);
	// Create the GPU Texture
	void 			Create();
	// Get ready to stop using the texture, for destruction on next frame
	void			MarkForDestroy();
	// On pending delete, destroy the GPU texture and do some cleanup
	void 			Update();
	// Return true when GPU texture can be used for display
	bool 			IsValid() const;
	// Destroy the GPU texture
	void 			Destroy();
	// Utility method
	size_t 			GetSizeInBytes();

	ImTextureID				mTextureId = ImTextureID_Invalid;	// Texture ID to use with Dear ImGui API
	void* 					mTextureObject = nullptr;			// Pointer to the actual GPU texture resource
	ImVector<uint8_t>		mPixels;							// Byte array for pixel data used during GPU texture init
	NetImgui::eTexFormat 	mFormat = NetImgui::eTexFormat::kTexFmt_Invalid;
	uint16_t				mWidth = 0u;
	uint16_t				mHeight = 0u;
	bool 					mDestroyOnNextFrame = false;
	bool 					mIsSentToServer = false;	
};
#endif