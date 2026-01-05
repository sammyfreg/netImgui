//=================================================================================================
// Source file handling renderer specific commands needed by NetImgui server.
// It complement the rendering backend with a few more functionalities.
// 
//  @note: Thank you to github user A3Wypiok, for a lot of the OpenGL found in this file
//=================================================================================================
#include "NetImguiServer_App.h"

#if HAL_API_PLATFORM_GLFW_GL3

#include "NetImguiServer_RemoteClient.h"
#include <imgui_impl_opengl3.h>
#include <imgui_impl_opengl3_loader.h>

namespace NetImguiServer { namespace App
{

//=================================================================================================
// HAL RENDER DRAW DATA
//=================================================================================================
void HAL_RenderDrawData(ImDrawData* pDrawData)
{
	ImGui_ImplOpenGL3_RenderDrawData(pDrawData);
}

//=================================================================================================
// HAL RENDER DRAW DATA
// The drawing of remote clients is handled normally by the standard rendering backend,
// but output is redirected to an allocated client texture  instead default swapchain
//=================================================================================================
void HAL_RenderDrawData(RemoteClient::Client& client, ImDrawData* pDrawData)
{
	if( client.mpHAL_AreaRT )
	{
		glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(reinterpret_cast<uint64_t>(client.mpHAL_AreaRT)));
		glClearColor(client.mBGSettings.mClearColor[0], client.mBGSettings.mClearColor[1], client.mBGSettings.mClearColor[2], client.mBGSettings.mClearColor[3]);
		glClear(GL_COLOR_BUFFER_BIT);
		{
			void* mainBackend = ImGui::GetIO().BackendRendererUserData;
			NetImgui::Internal::ScopedImguiContext scopedCtx(client.mpBGContext);
			ImGui::GetIO().BackendRendererUserData = mainBackend; // Re-appropriate the existing renderer backend, for this client rendeering
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			ImGui::GetIO().BackendRendererUserData = nullptr;
		}
		if (pDrawData)
		{
			ImGui_ImplOpenGL3_RenderDrawData(pDrawData);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

    GLuint TextureView = 0u;
    glGenTextures(1, &TextureView);
    glBindTexture(GL_TEXTURE_2D, TextureView);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    GLuint pRenderTargetView = 0u;
    glGenFramebuffers(1, &pRenderTargetView);
    glBindFramebuffer(GL_FRAMEBUFFER, pRenderTargetView);

    //Attach 2D texture to this FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TextureView, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	bool bSuccess = status == GL_FRAMEBUFFER_COMPLETE;

	// Unbind resources
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0); 

	if( bSuccess ){
		pOutRT      = reinterpret_cast<void*>(static_cast<uint64_t>(pRenderTargetView));
		// Store identifiers
        OutTexture.SetTexID(static_cast<ImTextureID>(TextureView));
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
		GLuint pRT	= static_cast<GLuint>(reinterpret_cast<uint64_t>(pOutRT));
		pOutRT		= nullptr;
		glDeleteFramebuffers(1, &pRT);
	}
	if(OutTexture.Status != ImTextureStatus_Destroyed && OutTexture.GetTexID() != ImTextureID_Invalid)
	{
		GLuint ViewTexture = static_cast<GLuint>(OutTexture.GetTexID());
		glDeleteTextures(1, &ViewTexture);

		// Clear identifiers and mark as destroyed (in order to allow e.g. calling InvalidateDeviceObjects while running)
		OutTexture.SetTexID(ImTextureID_Invalid);
		OutTexture.SetStatus(ImTextureStatus_Destroyed);
	}
}

}} //namespace NetImguiServer { namespace App

#endif // HAL_API_PLATFORM_GLFW_GL3
