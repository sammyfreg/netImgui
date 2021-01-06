//=================================================================================================
// Source file handling renderer specific commands needed by NetImgui server.
// It complement the rendering backend with a few more functinoalities.
//=================================================================================================
#include "NetImguiServer_App.h"
#include <Private/NetImgui_Shared.h>
#include "imgui_impl_dx11.h"
#include <d3d11.h>

extern ID3D11Device*&           g_pd3dDeviceExtern;
extern ID3D11DeviceContext*&    g_pd3dDeviceContextExtern;

namespace NetImguiServer { namespace App
{

//=================================================================================================
// HAL RENDER DRAW DATA
// The drawing of remote clients is handled normally by the standard rendering backend,
// but output is redirected to an allocated client texture  instead default swapchain
//=================================================================================================
void HAL_RenderDrawData(void* pRT, ImDrawData* pDrawData, const float ClearColor[4])
{
    if( pRT )
    {
        g_pd3dDeviceContextExtern->OMSetRenderTargets(1, reinterpret_cast<ID3D11RenderTargetView**>(&pRT), NULL);
        g_pd3dDeviceContextExtern->ClearRenderTargetView(reinterpret_cast<ID3D11RenderTargetView*>(pRT), ClearColor);
        if (pDrawData)
        {
            ImGui_ImplDX11_RenderDrawData(pDrawData);
        }
    }
}

//=================================================================================================
// HAL RENDER STATE SETUP
// Before rendering a client's ImGui content, this function let us modify the current state. 
// Each remote client content is outputed into a rendertarget, and default alpha channel behavior
// doesn't suits our needs. 
//
// Note : The rendertarget results used as display texture during drawing of the 'NetImgui Server'  
//        UI, as a background of each client's window
//=================================================================================================
void HAL_RenderStateSetup()
{
    // We want the rendertarget to remember the most opaque value per pixel, 
    //when compositing it back inside our Imgui window
    static ID3D11BlendState* spBlendState = nullptr;
    if( spBlendState == nullptr )
    {
        D3D11_BLEND_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.AlphaToCoverageEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        g_pd3dDeviceExtern->CreateBlendState(&desc, &spBlendState);
    }

    ID3D11BlendState*   BlendState;
    FLOAT               BlendFactor[4];
    UINT                SampleMask;
    g_pd3dDeviceContextExtern->OMGetBlendState(&BlendState, BlendFactor, &SampleMask);
    g_pd3dDeviceContextExtern->OMSetBlendState(spBlendState, BlendFactor, SampleMask);
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
    HRESULT Result						= g_pd3dDeviceExtern->CreateTexture2D(&texDesc, nullptr, &pTexture);
    if( Result == S_OK )
	{
        Result = g_pd3dDeviceExtern->CreateRenderTargetView(pTexture, nullptr, &pRenderTargetView );
        if( Result == S_OK )
        {
		    srvDesc.Format						= texDesc.Format;
		    srvDesc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
		    srvDesc.Texture2D.MipLevels			= texDesc.MipLevels;
		    srvDesc.Texture2D.MostDetailedMip	= 0;
		    Result                              = g_pd3dDeviceExtern->CreateShaderResourceView(pTexture, &srvDesc, &pTextureView);
            
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
bool HAL_CreateTexture(uint16_t Width, uint16_t Height, NetImgui::eTexFormat Format, const uint8_t* pPixelData, void*& pOutTexture)
{
    HAL_DestroyTexture(pOutTexture);

    // Convert all incoming textures data to RGBA8
    uint32_t* pPixelDataAlloc = nullptr;    
    if(Format == NetImgui::eTexFormat::kTexFmtA8)
    {
        pPixelDataAlloc = NetImgui::Internal::netImguiSizedNew<uint32_t>(Width*Height*4);              
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
	D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem					= pPixelData;
    subResource.SysMemPitch				= Width * 4;
    subResource.SysMemSlicePitch		= 0;
    HRESULT Result						= g_pd3dDeviceExtern->CreateTexture2D(&texDesc, &subResource, &pTexture);
    if( Result == S_OK )
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format						= texFmt;
		srvDesc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels			= texDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip	= 0;
		g_pd3dDeviceExtern->CreateShaderResourceView(pTexture, &srvDesc, &pTextureView);            
        if( Result == S_OK )
        {
            pTexture->Release();
            pOutTexture = reinterpret_cast<void*>(pTextureView);
            return true;
        }
    }
   
    if( pTextureView ){
        pTextureView->Release();
    }    
    if( pTexture ){
        pTexture->Release();
    }
    NetImgui::Internal::netImguiDeleteSafe(pPixelDataAlloc);
	return false;
}

//=================================================================================================
// HAL DESTROY TEXTURE
// Free up allocated resources tried to a Texture
//=================================================================================================
void HAL_DestroyTexture(void*& pTexture)
{
    if( pTexture )
    {
        reinterpret_cast<ID3D11ShaderResourceView*>(pTexture)->Release();        
        pTexture = nullptr;
    }
}

}} //namespace NetImguiServer { namespace App
