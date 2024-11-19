//=================================================================================================
// Source file handling renderer specific commands needed by NetImgui server.
// It complement the rendering backend with a few more functionalities.
//=================================================================================================
#include "NetImguiServer_App.h"

#if HAL_API_PLATFORM_WIN32_DX11

#include "NetImguiServer_RemoteClient.h"
#include <Private/NetImgui_Shared.h>
#include "imgui_impl_dx11.h"
#include <d3d11.h>

namespace NetImguiServer { namespace App
{

//Copied over from 'backends\imgui_impl_dx11.cpp'
struct ImGui_ImplDX11_Data
{
    ID3D11Device*               pd3dDevice;
    ID3D11DeviceContext*        pd3dDeviceContext;
    // Removed other member we are not interested in
};

inline ID3D11Device* GetD3DDevice()
{
	return ImGui::GetIO().BackendRendererUserData ? reinterpret_cast<ImGui_ImplDX11_Data*>(ImGui::GetIO().BackendRendererUserData)->pd3dDevice : nullptr;
	//return reinterpret_cast<ImGui_ImplDX11_RenderState*>(ImGui::GetPlatformIO().Renderer_RenderState)->Device; // Only valid during ImGui_ImplDX11_RenderDrawData() atm
}

inline ID3D11DeviceContext* GetD3DContext()
{
	return ImGui::GetIO().BackendRendererUserData ? reinterpret_cast<ImGui_ImplDX11_Data*>(ImGui::GetIO().BackendRendererUserData)->pd3dDeviceContext : nullptr;
	//return reinterpret_cast<ImGui_ImplDX11_RenderState*>(ImGui::GetPlatformIO().Renderer_RenderState)->DeviceContext;
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
		GetD3DContext()->OMSetRenderTargets(1, reinterpret_cast<ID3D11RenderTargetView**>(&client.mpHAL_AreaRT), NULL);
		GetD3DContext()->ClearRenderTargetView(reinterpret_cast<ID3D11RenderTargetView*>(client.mpHAL_AreaRT), client.mBGSettings.mClearColor);
		{
			void* mainBackend = ImGui::GetIO().BackendRendererUserData;
			NetImgui::Internal::ScopedImguiContext scopedCtx(client.mpBGContext);
			ImGui::GetIO().BackendRendererUserData = mainBackend; 	// Appropriate the existing renderer backend, for this client rendering
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			ImGui::GetIO().BackendRendererUserData = nullptr;		// Restore it to null
		}
		if (pDrawData)
		{
			ImGui_ImplDX11_RenderDrawData(pDrawData);
		}
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
	
	ID3D11Texture2D*                pTexture(nullptr);
	ID3D11RenderTargetView*         pRenderTargetView(nullptr);
	ID3D11ShaderResourceView*       pTextureView(nullptr);
	D3D11_TEXTURE2D_DESC            texDesc;	
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	ZeroMemory(&srvDesc, sizeof(srvDesc));

	texDesc.Width						= static_cast<UINT>(Width);
	texDesc.Height						= static_cast<UINT>(Height);
	texDesc.MipLevels					= 1;
	texDesc.ArraySize					= 1;
	texDesc.Format						= DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count			= 1;
	texDesc.Usage						= D3D11_USAGE_DEFAULT;
	texDesc.BindFlags					= D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags				= 0;
	HRESULT Result						= GetD3DDevice()->CreateTexture2D(&texDesc, nullptr, &pTexture);
	if( Result == S_OK )
	{
		Result = GetD3DDevice()->CreateRenderTargetView(pTexture, nullptr, &pRenderTargetView );
		if( Result == S_OK )
		{
			srvDesc.Format						= texDesc.Format;
			srvDesc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels			= texDesc.MipLevels;
			srvDesc.Texture2D.MostDetailedMip	= 0;
			Result                              = GetD3DDevice()->CreateShaderResourceView(pTexture, &srvDesc, &pTextureView);
			
			if( Result == S_OK )
			{
				pTexture->Release();
				pOutRT      = reinterpret_cast<void*>(pRenderTargetView);
				pOutTexture = reinterpret_cast<void*>(pTextureView);
				return true;
			}
		}
	}
	
	if( pTextureView ){
		pTextureView->Release();
	}    
	if( pRenderTargetView ){
		pRenderTargetView->Release();
	}
	if( pTexture ){
		pTexture->Release();
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
		reinterpret_cast<ID3D11RenderTargetView*>(pOutRT)->Release();
		pOutRT = nullptr;
	}
	if(pOutTexture)
	{
		reinterpret_cast<ID3D11ShaderResourceView*>(pOutTexture)->Release();        
		pOutTexture = nullptr;
	}
}

//=================================================================================================
// HAL CREATE TEXTURE
// Receive info on a Texture to allocate. At the moment, 'Dear ImGui' default rendering backend
// only support RGBA8 format, so first convert any input format to a RGBA8 that we can use
//=================================================================================================
bool HAL_CreateTexture(uint16_t Width, uint16_t Height, NetImgui::eTexFormat Format, const uint8_t* pPixelData, ServerTexture& OutTexture)
{
	NetImguiServer::App::EnqueueHALTextureDestroy(OutTexture);

	// Convert all incoming textures data to RGBA8
	uint32_t* pPixelDataAlloc = nullptr;
	if(Format == NetImgui::eTexFormat::kTexFmtA8)
	{
		pPixelDataAlloc = NetImgui::Internal::netImguiSizedNew<uint32_t>(static_cast<size_t>(Width)*static_cast<size_t>(Height)*4);
		for (int i = 0; i < Width * Height; ++i){
			pPixelDataAlloc[i] = 0x00FFFFFF | (static_cast<uint32_t>(pPixelData[i])<<24);
		}
		pPixelData = reinterpret_cast<const uint8_t*>(pPixelDataAlloc);
	}
	else if (Format == NetImgui::eTexFormat::kTexFmtRGBA8)
	{
	}
	// Unsupported format
	else
	{
		return false;
	}

	// Create the texture buffer
	ID3D11Texture2D*            pTexture(nullptr);
	ID3D11ShaderResourceView*   pTextureView(nullptr);
	DXGI_FORMAT                 texFmt(DXGI_FORMAT_UNKNOWN);
	D3D11_TEXTURE2D_DESC        texDesc;
	D3D11_SUBRESOURCE_DATA		subResource;
	ZeroMemory(&texDesc, sizeof(texDesc));
	
	texDesc.Width						= Width;
	texDesc.Height						= Height;
	texDesc.MipLevels					= 1;
	texDesc.ArraySize					= 1;
	texDesc.Format						= DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count			= 1;
	texDesc.Usage						= D3D11_USAGE_DEFAULT;
	texDesc.BindFlags					= D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags				= 0;
	subResource.pSysMem					= pPixelData;
	subResource.SysMemPitch				= Width * 4;
	subResource.SysMemSlicePitch		= 0;
	HRESULT Result						= GetD3DDevice()->CreateTexture2D(&texDesc, &subResource, &pTexture);
	
	if( Result == S_OK )
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format						= texFmt;
		srvDesc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels			= texDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip	= 0;
		GetD3DDevice()->CreateShaderResourceView(pTexture, &srvDesc, &pTextureView);
		if( Result == S_OK )
		{
			pTexture->Release();
			OutTexture.mpHAL_Texture		= reinterpret_cast<void*>(pTextureView);
		}
	}
	
	if( Result != S_OK && pTextureView ){
		pTextureView->Release();
	}
	if( Result != S_OK && pTexture ){
		pTexture->Release();
	}
	NetImgui::Internal::netImguiDeleteSafe(pPixelDataAlloc);
	return Result == S_OK;
}

//=================================================================================================
// HAL DESTROY TEXTURE
// Free up allocated resources tried to a Texture
//=================================================================================================
void HAL_DestroyTexture(ServerTexture& OutTexture)
{
	if( OutTexture.mpHAL_Texture )
	{
		reinterpret_cast<ID3D11ShaderResourceView*>(OutTexture.mpHAL_Texture)->Release();
		memset(&OutTexture, 0, sizeof(OutTexture));
	}
}

}} //namespace NetImguiServer { namespace App

#endif // HAL_API_PLATFORM_WIN32_DX11