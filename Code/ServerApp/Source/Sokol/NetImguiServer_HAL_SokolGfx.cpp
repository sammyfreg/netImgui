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
//=================================================================================================
void HAL_RenderDrawData(ImDrawData* pDrawData)
{
	simgui_render_draw_data(pDrawData);
}

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
                ImGui::GetIO().BackendRendererUserData = nullptr;
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
bool HAL_CreateRenderTarget(uint16_t Width, uint16_t Height, void*& pOutRT, ImTextureData& OutTexture)
{
	HAL_DestroyRenderTarget(pOutRT, OutTexture);

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
		// Store identifiers
        OutTexture.SetTexID(static_cast<ImTextureID>(img.id));
        OutTexture.SetStatus(ImTextureStatus_OK);
		return true;
	}

    return false;
}

//=================================================================================================
// HAL DESTROY RENDER TARGET
// Free up allocated resources tried to a RenderTarget
//=================================================================================================
void HAL_DestroyRenderTarget(void*& pOutRT, ImTextureData& OutTexture)
{
    if(pOutRT)
	{
		sg_image pRT{};
        pRT.id = static_cast<uint32_t>(reinterpret_cast<uint64_t>(pOutRT));
		pOutRT = nullptr;
        sg_destroy_image(pRT);
	}

	if(OutTexture.Status != ImTextureStatus_Destroyed && OutTexture.GetTexID() != ImTextureID_Invalid)
	{
		sg_image pTexture{};
        pTexture.id = static_cast<uint32_t>(OutTexture.GetTexID());
        sg_destroy_image(pTexture);

		// Clear identifiers and mark as destroyed (in order to allow e.g. calling InvalidateDeviceObjects while running)
		OutTexture.SetTexID(ImTextureID_Invalid);
		OutTexture.SetStatus(ImTextureStatus_Destroyed);
	}
}

}} //namespace NetImguiServer { namespace App

#endif // HAL_API_PLATFORM_SOKOL
