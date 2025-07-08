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

extern void ImGui_ImplDX11_UpdateTexture(ImTextureData* tex);

namespace NetImguiServer { namespace App
{

//Copied over from 'backends\imgui_impl_dx11.cpp'
struct ImGui_ImplDX11_Texture
{
	ID3D11Texture2D*            pTexture;
	ID3D11ShaderResourceView*   pTextureView;
};

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
bool HAL_CreateRenderTarget(uint16_t Width, uint16_t Height, void*& pOutRT, ImTextureData& OutTexture)
{
	HAL_DestroyRenderTarget(pOutRT, OutTexture);

	ID3D11Texture2D*                pTexture(nullptr);
	ID3D11RenderTargetView*         pRenderTargetView(nullptr);
	ID3D11ShaderResourceView*       pTextureView(nullptr);
	D3D11_TEXTURE2D_DESC            texDesc;	
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	ZeroMemory(&srvDesc, sizeof(srvDesc));

	texDesc.Width				= static_cast<UINT>(Width);
	texDesc.Height				= static_cast<UINT>(Height);
	texDesc.MipLevels			= 1;
	texDesc.ArraySize			= 1;
	texDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count	= 1;
	texDesc.Usage				= D3D11_USAGE_DEFAULT;
	texDesc.BindFlags			= D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags		= 0;
	HRESULT Result				= GetD3DDevice()->CreateTexture2D(&texDesc, nullptr, &pTexture);
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
				pOutRT      						= reinterpret_cast<void*>(pRenderTargetView);
				OutTexture 							= ImTextureData();
				OutTexture.RefCount 				= 1;
				OutTexture.Format					= ImTextureFormat::ImTextureFormat_RGBA32;
				OutTexture.BytesPerPixel			= 4;				
				OutTexture.Height					= texDesc.Height;
				OutTexture.Width					= texDesc.Width;
				
				//-------------------------------------------------------------
				// Taken from ImGui_ImplDX11_UpdateTexture
				// Use the RenderTarget as the resource, no need to create a
				// texture, we only care about the ResourceView
				ImGui_ImplDX11_Texture* backend_tex = IM_NEW(ImGui_ImplDX11_Texture)();
				backend_tex->pTexture				= nullptr;
				backend_tex->pTextureView			= pTextureView;
				// Store identifiers
				OutTexture.SetTexID((ImTextureID)(intptr_t)backend_tex->pTextureView);
				OutTexture.SetStatus(ImTextureStatus_OK);
				OutTexture.BackendUserData 	= backend_tex;
				//-------------------------------------------------------------
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
void HAL_DestroyRenderTarget(void*& pOutRT, ImTextureData& OutTexture)
{
	if(pOutRT)
	{
		reinterpret_cast<ID3D11RenderTargetView*>(pOutRT)->Release();
		pOutRT = nullptr;
	}

	//-------------------------------------------------------------
	// Taken from ImGui_ImplDX11_DestroyTexture
	// Use the RenderTarget as the resource, no need to create a
	// texture, we only care about the ResourceView
	ImGui_ImplDX11_Texture* backend_tex = (ImGui_ImplDX11_Texture*)OutTexture.BackendUserData;
	if (backend_tex == nullptr)
		return;
	IM_ASSERT(backend_tex->pTextureView == (ID3D11ShaderResourceView*)(intptr_t)OutTexture.TexID);
	backend_tex->pTextureView->Release();
	IM_DELETE(backend_tex);

	// Clear identifiers and mark as destroyed (in order to allow e.g. calling InvalidateDeviceObjects while running)
	OutTexture.SetTexID(ImTextureID_Invalid);
	OutTexture.SetStatus(ImTextureStatus_Destroyed);
	OutTexture.BackendUserData = nullptr;
	//-------------------------------------------------------------
}

}} //namespace NetImguiServer { namespace App

#endif // HAL_API_PLATFORM_WIN32_DX11