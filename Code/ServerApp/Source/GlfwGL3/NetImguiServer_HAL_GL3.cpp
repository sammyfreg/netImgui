//=================================================================================================
// Source file handling renderer specific commands needed by NetImgui server.
// It complement the rendering backend with a few more functionalities.
// 
//  @note: Thank you to github user A3Wypiok, for a lot of the OpenGL found in this file
//=================================================================================================
#include "NetImguiServer_App.h"

#if HAL_API_PLATFORM_GLFW_GL3

#include <array>
#include "NetImguiServer_RemoteClient.h"
#include <imgui_impl_opengl3.h>
#include "imgui_impl_opengl3_loader.h"

namespace NetImguiServer { namespace App
{
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
bool HAL_CreateRenderTarget(uint16_t Width, uint16_t Height, void*& pOutRT, void*& pOutTexture)
{
	HAL_DestroyRenderTarget(pOutRT, pOutTexture);

    GLuint pTextureView = 0u;
    glGenTextures(1, &pTextureView);
    glBindTexture(GL_TEXTURE_2D, pTextureView);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		
    GLuint pRenderTargetView = 0u;
    glGenFramebuffers(1, &pRenderTargetView);
    glBindFramebuffer(GL_FRAMEBUFFER, pRenderTargetView);

    //Attach 2D texture to this FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pTextureView, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	bool bSuccess = status == GL_FRAMEBUFFER_COMPLETE;

	// Unbind resources
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0); 

	if( bSuccess ){
		pOutRT      = reinterpret_cast<void*>(static_cast<uint64_t>(pRenderTargetView));
		pOutTexture = reinterpret_cast<void*>(static_cast<uint64_t>(pTextureView));
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
		GLuint pRT	= static_cast<GLuint>(reinterpret_cast<uint64_t>(pOutRT));
		pOutRT		= nullptr;
		glDeleteFramebuffers(1, &pRT);
	}
	if(pOutTexture)
	{
		GLuint pTexture = static_cast<GLuint>(reinterpret_cast<uint64_t>(pOutTexture));
		pOutTexture		= nullptr;
		glDeleteTextures(1, &pTexture);
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

	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture); // Save state

	// Create new texture
	GLuint texture = 0u;
	glGenTextures(1, &texture);

	//GLenum error = glGetError(); (void)error;
	if( texture != 0 ){
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pPixelData);

		OutTexture.mSize[0]			= Width;
		OutTexture.mSize[1]			= Height;
		OutTexture.mpHAL_Texture	= reinterpret_cast<void*>(static_cast<uint64_t>(texture));
		NetImgui::Internal::netImguiDeleteSafe(pPixelDataAlloc);
	
		glBindTexture(GL_TEXTURE_2D, last_texture); // Restore state
	}
	return texture != 0;
}

//=================================================================================================
// HAL DESTROY TEXTURE
// Free up allocated resources tried to a Texture
//=================================================================================================
void HAL_DestroyTexture(ServerTexture& OutTexture)
{
	GLuint pTexture = static_cast<GLuint>(reinterpret_cast<uint64_t>(OutTexture.mpHAL_Texture));
	glDeleteTextures(1, &pTexture);
	memset(&OutTexture, 0, sizeof(OutTexture));
}

}} //namespace NetImguiServer { namespace App

#endif // HAL_API_PLATFORM_GLFW_GL3
