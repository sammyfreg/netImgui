#include "TextureResource.h"
#include "imgui_internal.h" // Needed for 'RegisterUserTexture/UnregisterUserTexture/ImMin/imMax'

// Methods extracted from DirectX11 backend and copied to main.cpp
extern void TextureCreate(const uint8_t* pPixelData, uint32_t width, uint32_t height, void*& pTextureViewOut);
extern void TextureDestroy(void*& pTextureView);


//#################################################################################################
//#################################################################################################
// DEAR IMGUI TEXTURE RESOURCE
//#################################################################################################
//#################################################################################################

//=================================================================================================
// Initialize a new native Dear ImGui texture entry that relies on the
// Dear ImGui Backend for resource creation and handling. 
// This mostly allocate the pixels data we will use to send to GPU texture resource
TexResImgui& TexResImgui::InitPixels(ImTextureFormat Format, int Width, int Height)
//=================================================================================================
{
	assert(Format == ImTextureFormat_RGBA32); //Note: Current backend only support rgba32 atm
	assert(mImTexRef._TexData == nullptr); // Do not double init

	// Init the new native ImGui texture support (for backend that support it)
	mImTexData.Create(Format, Width, Height);
	mImTexData.Status 		= ImTextureStatus_Destroyed;
	mImTexData.UseColors	= Format != ImTextureFormat_Alpha8; // Doesn't seem to do much, but might as well let the backend know
	mImTexRef._TexData 		= &mImTexData;
	return *this;
}

//=================================================================================================
void TexResImgui::Create()
//=================================================================================================
{
	assert(!IsValid());
	mImTexData.SetStatus(ImTextureStatus_WantCreate);
	mImTexData.RefCount = 1;
	ImGui::RegisterUserTexture(&mImTexData);
}

//=================================================================================================
void TexResImgui::MarkForDestroy()
//=================================================================================================
{
	assert(IsValid());
	mImTexData.WantDestroyNextFrame = true;
}

//=================================================================================================
void TexResImgui::Update()
//=================================================================================================
{
	// Texture has been flagged for destruction
	if( mImTexData.WantDestroyNextFrame )
	{
		mImTexData.SetStatus(ImTextureStatus_WantDestroy);
		mImTexData.WantDestroyNextFrame = false;
	}
	// Let the backend know that texture can be released
	else if(mImTexData.Status == ImTextureStatus_WantDestroy)
	{
		mImTexData.UnusedFrames++;
	}
	// When backend is done destroying the texture, remove it from user list
	else if( mImTexData.Status == ImTextureStatus_Destroyed && mImTexData.RefCount != 0 )
	{
		ImGui::UnregisterUserTexture(&mImTexData);
		mImTexData.RefCount = 0;
	}
}

//=================================================================================================
// Let the Backend know that part of this texture content has been updated
// Note: The code is 95% taken straight from the Dear ImGui code base
void TexResImgui::MarkUpdated(int x, int y, int w, int h)
//=================================================================================================
{
	ImTextureData* tex = &mImTexData;
	assert(tex->Pixels != nullptr); // Must have been init once
	
	//-------------------------------------------------------------------------
	// Copied as is from the 'ImFontAtlasTextureBlockQueueUpload' function
	IM_ASSERT(tex->Status != ImTextureStatus_WantDestroy && tex->Status != ImTextureStatus_Destroyed);
    IM_ASSERT(x >= 0 && x <= 0xFFFF && y >= 0 && y <= 0xFFFF && w >= 0 && x + w <= 0x10000 && h >= 0 && y + h <= 0x10000);

	ImTextureRect req = { (unsigned short)x, (unsigned short)y, (unsigned short)w, (unsigned short)h };
    int new_x1 = ImMax(tex->UpdateRect.w == 0 ? 0 : tex->UpdateRect.x + tex->UpdateRect.w, req.x + req.w);
    int new_y1 = ImMax(tex->UpdateRect.h == 0 ? 0 : tex->UpdateRect.y + tex->UpdateRect.h, req.y + req.h);
    tex->UpdateRect.x = ImMin(tex->UpdateRect.x, req.x);
    tex->UpdateRect.y = ImMin(tex->UpdateRect.y, req.y);
    tex->UpdateRect.w = (unsigned short)(new_x1 - tex->UpdateRect.x);
    tex->UpdateRect.h = (unsigned short)(new_y1 - tex->UpdateRect.y);
    tex->UsedRect.x = ImMin(tex->UsedRect.x, req.x);
    tex->UsedRect.y = ImMin(tex->UsedRect.y, req.y);
    tex->UsedRect.w = (unsigned short)(ImMax(tex->UsedRect.x + tex->UsedRect.w, req.x + req.w) - tex->UsedRect.x);
    tex->UsedRect.h = (unsigned short)(ImMax(tex->UsedRect.y + tex->UsedRect.h, req.y + req.h) - tex->UsedRect.y);
  	//atlas->TexIsBuilt = false;

    // No need to queue if status is _WantCreate
    if (tex->Status == ImTextureStatus_OK || tex->Status == ImTextureStatus_WantUpdates)
    {
        tex->Status = ImTextureStatus_WantUpdates;
        tex->Updates.push_back(req);
    }
}

//=================================================================================================
bool TexResImgui::IsValid() const
//=================================================================================================
{ 
	return	(mImTexData.GetTexID() != ImTextureID_Invalid) &&
			(mImTexData.WantDestroyNextFrame == false) &&
			(mImTexData.Status == ImTextureStatus_OK || mImTexData.Status == ImTextureStatus_WantUpdates);
}

//#################################################################################################
//#################################################################################################
// USER TEXTURE RESOURCE
//#################################################################################################
//#################################################################################################

#if NETIMGUI_ENABLED
//=================================================================================================
// Initialize a new User Texture entry that relies on manual management to display the textures.
// This mostly allocate the pixels data we will use to send to GPU texture resource	
TexResUser& TexResUser::InitPixels(NetImgui::eTexFormat Format, uint16_t width, uint16_t height)
//=================================================================================================
{
	assert(mPixels.Size == 0 && width > 0 && height > 0);
	assert(Format == NetImgui::eTexFormat::kTexFmtRGBA8 || Format == NetImgui::eTexFormat::kTexFmtA8);
	mFormat	= Format;
	mWidth	= width;
	mHeight = height;
	mPixels.resize(NetImgui::GetTexture_BytePerImage(Format, width, height));
	return *this;
}

//=================================================================================================
// Create the GPU Texture resource (if format is supported) from pixels data we allocated earlier
// and also send the texture data info to the NetImgui Server.
// Note: Unlike Dear ImGui managed textures, we have to manually let NetImgui know about it
void TexResUser::Create()
//=================================================================================================
{
	assert(mPixels.Size > 0 && !mTextureObject && !mIsSentToServer);

	// For local display
	// Note: Only supports RGBA8 format at the moment
	if( mFormat == NetImgui::eTexFormat::kTexFmtRGBA8 )
	{
		TextureCreate(mPixels.Data, mWidth, mHeight, mTextureObject);
	}

	// Utility union to safely convert a void* to ID (no matter the sizeof(void*))
	union CastHelper{ 
		CastHelper(const void* TexObj){ Uint = 0; Ptr = TexObj; } 
		uint64_t Uint; const void* Ptr; ImTextureID ID; 
	};
	// Use local texture id if available, otherwise create new id from this object address
	mTextureId 		= mTextureObject ? CastHelper(mTextureObject).ID : CastHelper(this).ID;
	mIsSentToServer = true;

	// For remote display
	NetImgui::SendDataTexture(mTextureId, mPixels.Data, mWidth, mHeight, mFormat);
}

//=================================================================================================
// Let the system know we want to release this texture
void TexResUser::MarkForDestroy()
//=================================================================================================
{
	assert(mTextureObject);
	mDestroyOnNextFrame = true;
}

//=================================================================================================
// Destroy the GPU texture resource and let NetImgui know that it should also destroy the texture.
// We're leaving the pixels data intact, to easily recreate the texture when requested.
void TexResUser::Destroy()
//=================================================================================================
{
	if( mTextureObject ){
		TextureDestroy(mTextureObject);
	}

	NetImgui::SendDataTexture(mTextureId, nullptr, 0, 0, NetImgui::eTexFormat::kTexFmt_Invalid);
	mTextureObject 		= nullptr;
	mTextureId 			= ImTextureID_Invalid;
	mDestroyOnNextFrame = false;
	mIsSentToServer		= false;
}

//=================================================================================================
// Destroy the GPU Texture if when detecting a pending destroy request
void TexResUser::Update()
//=================================================================================================
{
	// Texture has been flagged for destruction
	if( mDestroyOnNextFrame )
	{
		Destroy();
	}
}

//=================================================================================================
//Note: It is possible for the texture to be only valid on the remote NetImgui Server,
// 		when a locally unsuported format was used when initialized
bool TexResUser::IsValid() const
//=================================================================================================
{ 
	return !mDestroyOnNextFrame && mIsSentToServer && (mTextureObject || NetImgui::IsConnected());
}

//=================================================================================================
size_t TexResUser::GetSizeInBytes()
//=================================================================================================
{
	return NetImgui::GetTexture_BytePerImage(mFormat, mWidth, mHeight);
}

#endif // NETIMGUI_ENABLED

