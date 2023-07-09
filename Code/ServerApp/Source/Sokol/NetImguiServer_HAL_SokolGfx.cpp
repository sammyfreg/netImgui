//=================================================================================================
// Source file handling renderer specific commands needed by NetImgui server.
// It complement the rendering backend with a few more functionalities.
// 
//  @note: Thank you to github user A3Wypiok, for a lot of the OpenGL found in this file
//=================================================================================================
#include "NetImguiServer_App.h"

#if HAL_API_PLATFORM_SOKOL

#include "NetImguiServer_RemoteClient.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/util/sokol_imgui.h"

namespace NetImguiServer { namespace App
{
//=================================================================================================
// HAL RENDER DRAW DATA
// The drawing of remote clients is handled normally by the standard rendering backend,
// but output is redirected to an allocated client texture  instead default swapchain
//=================================================================================================
void HAL_RenderDrawData(RemoteClient::Client& client, ImDrawData* pDrawData)
{
    if (client.mpHAL_AreaRT)
    {
        sg_pass_action pass_action{};
        sg_pass_desc pass_desc{};

        pass_desc.color_attachments[0].image.id = static_cast<uint32_t>(reinterpret_cast<uint64_t>(client.mpHAL_AreaRT));
        pass_desc.depth_stencil_attachment.image.id = SG_INVALID_ID;
        pass_desc.label = "mpHAL_AreaRT";

        pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
        pass_action.colors[0].clear_value.r = client.mBGSettings.mClearColor[0];
        pass_action.colors[0].clear_value.g = client.mBGSettings.mClearColor[1];
        pass_action.colors[0].clear_value.b = client.mBGSettings.mClearColor[2];
        pass_action.colors[0].clear_value.a = client.mBGSettings.mClearColor[3];

        sg_pass pass = sg_make_pass(&pass_desc);

        sg_begin_pass(pass, &pass_action);
        {
            {
                void* mainBackend = ImGui::GetIO().BackendRendererUserData;
                NetImgui::Internal::ScopedImguiContext scopedCtx(client.mpBGContext);
                ImGui::GetIO().BackendRendererUserData = mainBackend; // Re-appropriate the existing renderer backend, for this client rendeering
                simgui_render_draw_data(ImGui::GetDrawData());
            }
            if (pDrawData)
            {
                simgui_render_draw_data(pDrawData);
            }
        }
        sg_end_pass();

        sg_destroy_pass(pass);
    }
}

//=================================================================================================
// HAL CREATE RENDER TARGET
// Allocate RenderTargetView / TextureView for each connected remote client. 
// The drawing of their menu content will be outputed in it, then displayed normally 
// inside our own 'NetImGui application' Imgui drawing
//=================================================================================================
bool HAL_CreateRenderTarget(uint16_t Width, uint16_t Height, void*& pOutRT, void*& pOutTexture)
{
	HAL_DestroyRenderTarget(pOutRT, pOutTexture);

    sg_image_desc img_desc{};
    img_desc.render_target = true;
    img_desc.width = Width;
    img_desc.height = Height;
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    img_desc.min_filter = SG_FILTER_LINEAR;
    img_desc.mag_filter = SG_FILTER_LINEAR;

    sg_image img = sg_make_image(&img_desc);

    bool bSuccess = img.id != SG_INVALID_ID;
    if( bSuccess ){
		pOutRT      = reinterpret_cast<void*>(static_cast<uint64_t>(img.id));
		pOutTexture = reinterpret_cast<void*>(static_cast<uint64_t>(img.id));
		return true;
	}

    return false;
}

//=================================================================================================
// HAL DESTROY RENDER TARGET
// Free up allocated resources tried to a RenderTarget
//=================================================================================================
void HAL_DestroyRenderTarget(void*& pOutRT, void*& pOutTexture)
{
    if(pOutRT)
	{
		sg_image pRT{};
        pRT.id = static_cast<uint32_t>(reinterpret_cast<uint64_t>(pOutRT));
		pOutRT = nullptr;
        sg_destroy_image(pRT);
	}
	if(pOutTexture)
	{
		sg_image pTexture{};
        pTexture.id = static_cast<uint32_t>(reinterpret_cast<uint64_t>(pOutTexture));
		pOutTexture = nullptr;
        sg_destroy_image(pTexture);
	}
}

//=================================================================================================
// HAL CREATE TEXTURE
// Receive info on a Texture to allocate. At the moment, 'Dear ImGui' default rendering backend
// only support RGBA8 format, so first convert any input format to a RGBA8 that we can use
//=================================================================================================
bool HAL_CreateTexture(uint16_t Width, uint16_t Height, NetImgui::eTexFormat Format, const uint8_t* pPixelData, ServerTexture& OutTexture)
{
	HAL_DestroyTexture(OutTexture);

	// Convert all incoming textures data to RGBA8
	uint32_t* pPixelDataAlloc = NetImgui::Internal::netImguiSizedNew<uint32_t>(Width*Height*4);
	if(Format == NetImgui::eTexFormat::kTexFmtA8)
	{
		for (int i = 0; i < Width * Height; ++i){
			pPixelDataAlloc[i] = 0x00FFFFFF | (static_cast<uint32_t>(pPixelData[i])<<24);
		}
		pPixelData = reinterpret_cast<const uint8_t*>(pPixelDataAlloc);
	}
	else if (Format == NetImgui::eTexFormat::kTexFmtRGBA8)
	{
	}
	else
	{
		// Unsupported format
		return false;
	}

    sg_image_desc img_desc{};
    img_desc.width = Width;
    img_desc.height = Height;
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    img_desc.usage = SG_USAGE_STREAM;

    sg_image img = sg_make_image(&img_desc);

    if (img.id != SG_INVALID_ID)
    {
        sg_image_data img_data{};
        img_data.subimage[0][0].ptr  = pPixelData;
        img_data.subimage[0][0].size = Width * Height * sizeof(uint32_t);
        sg_update_image(img, img_data);

        OutTexture.mSize[0]         = Width;
        OutTexture.mSize[1]         = Height;
        OutTexture.mpHAL_Texture	= reinterpret_cast<void*>(static_cast<uint64_t>(img.id));
    }

    NetImgui::Internal::netImguiDeleteSafe(pPixelDataAlloc);
    return img.id != SG_INVALID_ID;
}

//=================================================================================================
// HAL DESTROY TEXTURE
// Free up allocated resources tried to a Texture
//=================================================================================================
void HAL_DestroyTexture(ServerTexture& OutTexture)
{
    if(OutTexture.mpHAL_Texture)
    {
        sg_image image{};
        image.id = static_cast<uint32_t>(reinterpret_cast<uint64_t>(OutTexture.mpHAL_Texture));
        sg_destroy_image(image);
        OutTexture.mpHAL_Texture = nullptr;
    }
}

}} //namespace NetImguiServer { namespace App

#endif // HAL_API_PLATFORM_SOKOL
