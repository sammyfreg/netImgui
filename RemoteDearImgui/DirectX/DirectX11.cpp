
#include "stdafx.h"
#include "DirectX11.h"
#include <vector>
#include <d3d11.h>
#include <ClearVS.h>
#include <ClearPS.h>
#include <ImGuiVS.h>
#include <ImGuiPS.h>
#include <RemoteImgui_ImguiFrame.h>
#include <RemoteImgui_CmdPackets.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../ThirdParty/stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

#pragma comment (lib, "D3D11.lib") 

namespace dx
{

template <typename DXRestype>
class TDXResource
{
public:
	~TDXResource()				{ Release(); }
	DXRestype** GetForInit()	{ Release(); return &mpResource; }
	DXRestype*	Get()			{ return mpResource; }
	DXRestype**	GetArray()		{ return &mpResource; }
	DXRestype*	operator->()	{ return mpResource; }
	void Release()				{ if(mpResource) mpResource->Release(); mpResource = nullptr; }
protected:
	DXRestype* mpResource = nullptr;
};

template <typename DXBufferType, typename DXViewType>
struct TDXBufferViewRes
{	
	TDXResource<DXBufferType>	mBuffer;
	TDXResource<DXViewType>		mView;
};

struct ResShaderRes
{
	TDXResource<ID3D11InputLayout>	mInputLayout;
	TDXResource<ID3D11VertexShader>	mShaderVS;
	TDXResource<ID3D11PixelShader>	mShaderPS;
};

using ResCBuffer	= TDXResource<ID3D11Buffer>;
using ResVtxBuffer	= TDXResource<ID3D11Buffer>;
using ResIdxBuffer	= TDXResource<ID3D11Buffer>;
using ResTexture2D	= TDXBufferViewRes<ID3D11Texture2D, ID3D11ShaderResourceView>;

struct GfxResources
{	
	HWND								mhWindow;
	UINT								mFrameIndex;
	UINT								mScreenWidth;
	UINT								mScreenHeight;
	TDXResource<ID3D11Device>			mDevice;
	TDXResource<IDXGISwapChain>			mSwapChain;	
	TDXResource<ID3D11DeviceContext>	mContext;	
	TDXResource<ID3D11RenderTargetView>	mBackbufferView;
	TDXResource<ID3D11RasterizerState>	mDefaultRasterState;	
	TDXResource<ID3D11SamplerState>		mDefaultSampler;
	TDXResource<ID3D11Buffer>			mClearVertexBuffer;
	TDXResource<ID3D11BlendState>		mClearBlendState;
	ResShaderRes						mClearShaders;
	TDXResource<ID3D11BlendState>		mImguiBlendState;
	ResShaderRes						mImguiShaders;	
	ResTexture2D						mBackgroundTex;
};

GfxResources* gpGfxRes = nullptr;
std::vector<ResTexture2D> gTextures;

//SF TODO move this to other CreateTexture
bool CreateTexture2( ResTexture2D& OutTexture, int Width, int Height, int Channel, stbi_uc* pPixelData )
{
	// Create the texture buffer
	D3D11_TEXTURE2D_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(texDesc));	
	DXGI_FORMAT Format					=	Channel == 1 ?	DXGI_FORMAT_R8_UNORM :
											Channel == 2 ?	DXGI_FORMAT_R8G8_UNORM :
											Channel == 3 ?	DXGI_FORMAT_B8G8R8X8_UNORM : //need to swizzle this
															DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.Width						= Width;
    texDesc.Height						= Height;
    texDesc.MipLevels					= 1;
    texDesc.ArraySize					= 1;
    texDesc.Format						= Format;
    texDesc.SampleDesc.Count			= 1;
    texDesc.Usage						= D3D11_USAGE_DEFAULT;
    texDesc.BindFlags					= D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags				= 0;
	D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem					= pPixelData;
    subResource.SysMemPitch				= texDesc.Width * Channel;
    subResource.SysMemSlicePitch		= 0;
    HRESULT Result = gpGfxRes->mDevice->CreateTexture2D(&texDesc, &subResource, OutTexture.mBuffer.GetForInit());

    // Create texture view
	if( Result == S_OK )
	{    
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format						= Format;
		srvDesc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels			= texDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip	= 0;
		gpGfxRes->mDevice->CreateShaderResourceView(OutTexture.mBuffer.Get(), &srvDesc, OutTexture.mView.GetForInit());
	 }   
	return Result == S_OK;
}

bool CreateTexture(ResTexture2D& OutTexture, RmtImgui::eTexFormat format, uint16_t width, uint16_t height, const uint8_t* pPixelData)
{
// Create the texture buffer
	D3D11_TEXTURE2D_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(texDesc));	
	DXGI_FORMAT Format					= format == RmtImgui::kTexFmtRGB ? DXGI_FORMAT_B8G8R8X8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
	UINT BytesPerPixel					= format == RmtImgui::kTexFmtRGB ? 3 : 4;
    texDesc.Width						= width;
    texDesc.Height						= height;
    texDesc.MipLevels					= 1;
    texDesc.ArraySize					= 1;
    texDesc.Format						= Format;
    texDesc.SampleDesc.Count			= 1;
    texDesc.Usage						= D3D11_USAGE_DEFAULT;
    texDesc.BindFlags					= D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags				= 0;
	D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem					= pPixelData;
    subResource.SysMemPitch				= (UINT)texDesc.Width * BytesPerPixel;
    subResource.SysMemSlicePitch		= 0;
    HRESULT Result						= gpGfxRes->mDevice->CreateTexture2D(&texDesc, &subResource, OutTexture.mBuffer.GetForInit());

    // Create texture view
	if( Result == S_OK )
	{    
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format						= Format;
		srvDesc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels			= texDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip	= 0;
		gpGfxRes->mDevice->CreateShaderResourceView(OutTexture.mBuffer.Get(), &srvDesc, OutTexture.mView.GetForInit());
	}   
	return Result == S_OK;
}

void CreateCBuffer(ResCBuffer& ConstantBuffer, void* pData, UINT DataSize)
{
	D3D11_BUFFER_DESC Desc;
	D3D11_SUBRESOURCE_DATA subResource;
	Desc.ByteWidth					= DataSize;
	Desc.Usage						= D3D11_USAGE_DYNAMIC;
	Desc.BindFlags					= D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags				= D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags					= 0;
	Desc.StructureByteStride		= 0;
    subResource.pSysMem				= pData;
    subResource.SysMemPitch			= DataSize;
    subResource.SysMemSlicePitch	= 0;
	gpGfxRes->mDevice->CreateBuffer(&Desc, &subResource, ConstantBuffer.GetForInit());
}

void CreateVtxBuffer(ResVtxBuffer& VertexBuffer, void* pData, UINT DataSize)
{
	D3D11_BUFFER_DESC Desc;
	D3D11_SUBRESOURCE_DATA subResource;
	Desc.ByteWidth					= DataSize;
	Desc.Usage						= D3D11_USAGE_DYNAMIC;
	Desc.BindFlags					= D3D11_BIND_VERTEX_BUFFER;
	Desc.CPUAccessFlags				= D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags					= 0;
	Desc.StructureByteStride		= 0;
    subResource.pSysMem				= pData;
    subResource.SysMemPitch			= DataSize;
    subResource.SysMemSlicePitch	= 0;
	gpGfxRes->mDevice->CreateBuffer(&Desc, &subResource, VertexBuffer.GetForInit());
}

void CreateIdxBuffer(ResIdxBuffer& IndexBuffer, void* pData, UINT DataSize)
{
	D3D11_BUFFER_DESC Desc;
	D3D11_SUBRESOURCE_DATA subResource;
	Desc.ByteWidth					= DataSize;
	Desc.Usage						= D3D11_USAGE_DYNAMIC;
	Desc.BindFlags					= D3D11_BIND_INDEX_BUFFER;
	Desc.CPUAccessFlags				= D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags					= 0;
	Desc.StructureByteStride		= 0;
    subResource.pSysMem				= pData;
    subResource.SysMemPitch			= DataSize;
    subResource.SysMemSlicePitch	= 0;
	gpGfxRes->mDevice->CreateBuffer(&Desc, &subResource, IndexBuffer.GetForInit());
}

bool CreateShaderBinding(ResShaderRes& OutShaderBind, const BYTE* VS_Data, UINT VS_Size, const BYTE* PS_Data, UINT PS_Size, const D3D11_INPUT_ELEMENT_DESC* Layout, UINT LayoutCount )
{	
	HRESULT hr = gpGfxRes->mDevice->CreateVertexShader(VS_Data, VS_Size, nullptr, OutShaderBind.mShaderVS.GetForInit());
	if( hr != S_OK )
		return false;

	hr = gpGfxRes->mDevice->CreatePixelShader(PS_Data, PS_Size, nullptr, OutShaderBind.mShaderPS.GetForInit());
	if( hr != S_OK )
		return false;

	hr = gpGfxRes->mDevice->CreateInputLayout(Layout, LayoutCount, VS_Data, VS_Size, OutShaderBind.mInputLayout.GetForInit() );
	if( hr != S_OK )
		return false;
	return true;
}

bool Startup(HWND hWindow)
{
	if( gpGfxRes != nullptr )
		return false;

	HRESULT hr;
	RECT rcClient;
    GetClientRect(hWindow, &rcClient); 
	gpGfxRes										= new GfxResources();
	gpGfxRes->mhWindow								= hWindow;
	gpGfxRes->mFrameIndex							= 0;
	gpGfxRes->mScreenWidth							= rcClient.right - rcClient.left;
	gpGfxRes->mScreenHeight							= rcClient.bottom - rcClient.top;

	//Create our Device and SwapChain
	{
		DXGI_SWAP_CHAIN_DESC SwapDesc; 
		ZeroMemory(&SwapDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
		SwapDesc.BufferCount						= 2;
		SwapDesc.BufferDesc.Width					= gpGfxRes->mScreenWidth;
		SwapDesc.BufferDesc.Height					= gpGfxRes->mScreenHeight;
		SwapDesc.BufferDesc.Format					= DXGI_FORMAT_R8G8B8A8_UNORM;
		SwapDesc.BufferDesc.RefreshRate.Numerator	= 60;
		SwapDesc.BufferDesc.RefreshRate.Denominator	= 1;
		SwapDesc.SampleDesc.Count					= 1;
		SwapDesc.SampleDesc.Quality					= 0;
		SwapDesc.BufferUsage						= DXGI_USAGE_RENDER_TARGET_OUTPUT;		
		SwapDesc.OutputWindow						= hWindow; 
		SwapDesc.Windowed							= TRUE; 
		SwapDesc.SwapEffect							= DXGI_SWAP_EFFECT_DISCARD;		
		SwapDesc.Flags								= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		D3D_FEATURE_LEVEL featureLevel;
		const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0, };
		hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, featureLevelArray, 1,
			D3D11_SDK_VERSION, &SwapDesc, gpGfxRes->mSwapChain.GetForInit(), gpGfxRes->mDevice.GetForInit(), &featureLevel, gpGfxRes->mContext.GetForInit());
	}		

	//Create our BackBuffer and RT view	
	if( hr == S_OK )
	{
		TDXResource<ID3D11Texture2D> BackBuffer;	
		hr = gpGfxRes->mSwapChain->GetBuffer(0, __uuidof( ID3D11Texture2D ), (void**)BackBuffer.GetForInit());
		if( hr == S_OK )
			hr = gpGfxRes->mDevice->CreateRenderTargetView(BackBuffer.Get(), NULL, gpGfxRes->mBackbufferView.GetForInit());
	}
			
	// Create the blending setup
	if( hr == S_OK )
    {
        D3D11_BLEND_DESC ImguiBlendDesc;
        ZeroMemory(&ImguiBlendDesc, sizeof(ImguiBlendDesc));
        ImguiBlendDesc.AlphaToCoverageEnable				= false;
        ImguiBlendDesc.RenderTarget[0].BlendEnable			= true;
        ImguiBlendDesc.RenderTarget[0].SrcBlend				= D3D11_BLEND_SRC_ALPHA;
        ImguiBlendDesc.RenderTarget[0].DestBlend			= D3D11_BLEND_INV_SRC_ALPHA;
        ImguiBlendDesc.RenderTarget[0].BlendOp				= D3D11_BLEND_OP_ADD;
        ImguiBlendDesc.RenderTarget[0].SrcBlendAlpha		= D3D11_BLEND_INV_SRC_ALPHA;
        ImguiBlendDesc.RenderTarget[0].DestBlendAlpha		= D3D11_BLEND_ZERO;
        ImguiBlendDesc.RenderTarget[0].BlendOpAlpha			= D3D11_BLEND_OP_ADD;
        ImguiBlendDesc.RenderTarget[0].RenderTargetWriteMask= D3D11_COLOR_WRITE_ENABLE_ALL;
        hr = gpGfxRes->mDevice->CreateBlendState(&ImguiBlendDesc, gpGfxRes->mImguiBlendState.GetForInit());

		CD3D11_BLEND_DESC BlendClearDesc(D3D11_DEFAULT);
		if( hr == S_OK )
			hr = gpGfxRes->mDevice->CreateBlendState(&BlendClearDesc, gpGfxRes->mClearBlendState.GetForInit());
    }

    // Create the rasterizer state
    if( hr == S_OK )
{
        CD3D11_RASTERIZER_DESC desc(D3D11_DEFAULT);
        desc.CullMode = D3D11_CULL_NONE;        
        hr = gpGfxRes->mDevice->CreateRasterizerState(&desc, gpGfxRes->mDefaultRasterState.GetForInit());
    }

	// Create sampler states
	if( hr == S_OK )
    {
        CD3D11_SAMPLER_DESC samplerDesc(D3D11_DEFAULT);
        hr = gpGfxRes->mDevice->CreateSamplerState(&samplerDesc, gpGfxRes->mDefaultSampler.GetForInit());		
    }


	// Create the Shader Bindings
	if( hr == S_OK )
	{
		D3D11_INPUT_ELEMENT_DESC ClearVtxLayout; // Empty
		if( !CreateShaderBinding( gpGfxRes->mClearShaders, gpShader_ClearVS, sizeof(gpShader_ClearVS), gpShader_ClearPS, sizeof(gpShader_ClearPS), &ClearVtxLayout, 0) )
			return false;

		D3D11_INPUT_ELEMENT_DESC ImguiVtxLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R16G16_UNORM,	0, (size_t)(&((RmtImgui::ImguiVert*)0)->mPos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R16G16_UNORM,  0, (size_t)(&((RmtImgui::ImguiVert*)0)->mUV),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,0, (size_t)(&((RmtImgui::ImguiVert*)0)->mColor), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		if( !CreateShaderBinding( gpGfxRes->mImguiShaders, gpShader_ImguiVS, sizeof(gpShader_ImguiVS), gpShader_ImguiPS, sizeof(gpShader_ImguiPS), ImguiVtxLayout, ARRAY_COUNT(ImguiVtxLayout) ) )
			return false;
	}

	if( hr == S_OK )
	{
		int Width(0), Height(0), Channel(0);
		stbi_uc* pBGPixels = stbi_load("Background.png", &Width, &Height, &Channel, 0);
		if( pBGPixels )
		{		
			CreateTexture2(gpGfxRes->mBackgroundTex, Width, Height, Channel,pBGPixels);
			delete[] pBGPixels;
		}
	}
	return true;
}

void Shutdown()
{
	if( gpGfxRes )
	{
		delete gpGfxRes;
		gpGfxRes = nullptr;
	}
}

void Render_UpdateWindowSize()
{
	RECT rcClient;
	GetClientRect(gpGfxRes->mhWindow, &rcClient); 
	UINT ScreenWidth	= rcClient.right - rcClient.left;
	UINT ScreenHeight	= rcClient.bottom - rcClient.top;
	if( gpGfxRes->mScreenWidth != ScreenWidth || gpGfxRes->mScreenHeight != ScreenHeight )
	{
		gpGfxRes->mScreenWidth	= ScreenWidth;
		gpGfxRes->mScreenHeight	= ScreenHeight;
		gpGfxRes->mContext->ClearState();
		gpGfxRes->mBackbufferView.Release();
		HRESULT hr = gpGfxRes->mSwapChain->ResizeBuffers(0, gpGfxRes->mScreenWidth, gpGfxRes->mScreenHeight, DXGI_FORMAT_UNKNOWN, 0);
		
		DXGI_SWAP_CHAIN_DESC sd;
		gpGfxRes->mSwapChain->GetDesc(&sd);
		D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
		ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
		render_target_view_desc.Format = sd.BufferDesc.Format;
		render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		TDXResource<ID3D11Texture2D> BackBuffer;	
		hr = gpGfxRes->mSwapChain->GetBuffer(0, __uuidof( ID3D11Texture2D ), (void**)BackBuffer.GetForInit());
		if( hr == S_OK )
			hr = gpGfxRes->mDevice->CreateRenderTargetView(BackBuffer.Get(), &render_target_view_desc, gpGfxRes->mBackbufferView.GetForInit());
	}
}

void Render_Clear()
{
	CD3D11_VIEWPORT vp(0.f, 0.f, (float)gpGfxRes->mScreenWidth, (float)gpGfxRes->mScreenHeight);
	const D3D11_RECT r = { 0, 0, (long)gpGfxRes->mScreenWidth, (long)gpGfxRes->mScreenHeight };

	ResCBuffer ClearCB;
	float ClearColor[4] = {0,0,0,0.75f};
	const float BlendFactor[4] = { 1.f, 1.f, 1.f, 1.f };
	CreateCBuffer(ClearCB, &ClearColor, sizeof(ClearColor));	
	gpGfxRes->mContext->OMSetRenderTargets(1, gpGfxRes->mBackbufferView.GetArray(), nullptr );
	gpGfxRes->mContext->OMSetBlendState(gpGfxRes->mClearBlendState.Get(), BlendFactor, 0xFFFFFFFF);
	gpGfxRes->mContext->RSSetViewports(1, &vp);
	gpGfxRes->mContext->RSSetScissorRects(1, &r);
	gpGfxRes->mContext->RSSetState(gpGfxRes->mDefaultRasterState.Get());
	gpGfxRes->mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

//	unsigned int stride = 0;
//	unsigned int offset = 0;
	gpGfxRes->mContext->IASetInputLayout(gpGfxRes->mClearShaders.mInputLayout.Get());
	gpGfxRes->mContext->VSSetShader(gpGfxRes->mClearShaders.mShaderVS.Get(), nullptr, 0);
	gpGfxRes->mContext->PSSetShader(gpGfxRes->mClearShaders.mShaderPS.Get(), nullptr, 0);
	gpGfxRes->mContext->PSSetConstantBuffers(0, 1, ClearCB.GetArray());
	gpGfxRes->mContext->PSSetSamplers(0, 1, gpGfxRes->mDefaultSampler.GetArray());
	gpGfxRes->mContext->PSSetShaderResources(0, 1, gpGfxRes->mBackgroundTex.mView.GetArray());
 	gpGfxRes->mContext->Draw(4,0);
}

void Render_DrawImgui(const std::vector<TextureHandle>& textures, const RmtImgui::ImguiFrame* pDrawInfo)
{
	if( !pDrawInfo )
		return;
	
	const float L = 0.0f;
	const float R = (float)gpGfxRes->mScreenWidth;
	const float B = (float)gpGfxRes->mScreenHeight;
	const float T = 0.0f;
	const float mvp[4][4] = 
	{
	    { 2.0f/(R-L),   0.0f,           0.0f,       0.0f},
	    { 0.0f,         2.0f/(T-B),     0.0f,       0.0f,},
	    { 0.0f,         0.0f,           0.5f,       0.0f },
	    { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
	};

	const float BlendFactor[4] = { 1.f, 1.f, 1.f, 1.f };
	const unsigned int stride = sizeof(RmtImgui::ImguiVert);
	const unsigned int offset = 0;
	ResCBuffer VertexCB;
	ResVtxBuffer VertexBuffer;
	ResVtxBuffer IndexBuffer;
	CreateCBuffer(VertexCB, (void*)mvp, sizeof(mvp));
	CreateVtxBuffer(VertexBuffer, pDrawInfo->mpVertices, pDrawInfo->mVerticeCount*sizeof(RmtImgui::ImguiVert));
	CreateIdxBuffer(IndexBuffer, pDrawInfo->mpIndices, pDrawInfo->mIndiceSize);

	gpGfxRes->mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gpGfxRes->mContext->OMSetBlendState(gpGfxRes->mImguiBlendState.Get(), BlendFactor, 0xFFFFFFFF);
	gpGfxRes->mContext->IASetInputLayout(gpGfxRes->mImguiShaders.mInputLayout.Get());
	gpGfxRes->mContext->VSSetShader(gpGfxRes->mImguiShaders.mShaderVS.Get(), nullptr, 0);
	gpGfxRes->mContext->PSSetShader(gpGfxRes->mImguiShaders.mShaderPS.Get(), nullptr, 0);
	gpGfxRes->mContext->VSSetConstantBuffers(0, 1, VertexCB.GetArray());
	gpGfxRes->mContext->IASetVertexBuffers(0, 1, VertexBuffer.GetArray(), &stride, &offset);
	
	CD3D11_RECT ViewportPrev(0,0,0,0);
	uint64_t lastTextureId = (uint64_t)-1;
	for(unsigned int i(0); i<pDrawInfo->mDrawCount; ++i)
	{
		const auto* pDraw		= &pDrawInfo->mpDraws[i];
		CD3D11_RECT Viewport((LONG)pDraw->mClipRect[0], (LONG)pDraw->mClipRect[1], (LONG)pDraw->mClipRect[2], (LONG)pDraw->mClipRect[3] );		
		if( i == 0 || pDraw->mIndexSize != pDrawInfo->mpDraws[i-1].mIndexSize )
		{			
			gpGfxRes->mContext->IASetIndexBuffer(IndexBuffer.Get(), pDraw->mIndexSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
		}

		if( i == 0 || Viewport != ViewportPrev )
		{
			ViewportPrev = Viewport;
			gpGfxRes->mContext->RSSetScissorRects(1, &Viewport);
		}

		if( i == 0 || lastTextureId == pDraw->mTextureId )
		{
			lastTextureId = pDraw->mTextureId;
			ResTexture2D* pTexture = nullptr;
			for(int j(0); !pTexture && j<textures.size(); ++j)
			{
				if( textures[j].mImguiId == lastTextureId && 
					textures[j].mIndex < gTextures.size() &&
					gTextures[textures[j].mIndex].mBuffer.Get() != nullptr )
				{
					pTexture = &gTextures[textures[j].mIndex];
				}
			}				
			if( pTexture )
				gpGfxRes->mContext->PSSetShaderResources(0, 1, pTexture->mView.GetArray() );
		}

		gpGfxRes->mContext->DrawIndexed(pDraw->mIdxCount, pDraw->mIdxOffset/pDraw->mIndexSize, pDraw->mVtxOffset);
	}
}

void Render(const std::vector<TextureHandle>& textures, const RmtImgui::ImguiFrame* pDrawInfo)
{
	if( !gpGfxRes )
		return;
	
	Render_UpdateWindowSize();
	Render_Clear();
	Render_DrawImgui(textures, pDrawInfo);
		
	ID3D11RenderTargetView* RenderTargetNone[4]={nullptr,nullptr,nullptr,nullptr};
	gpGfxRes->mContext->OMSetRenderTargets(ARRAY_COUNT(RenderTargetNone), RenderTargetNone, nullptr );
	gpGfxRes->mSwapChain->Present(0, 0);		
	gpGfxRes->mFrameIndex++;
}

//SF safe multithread this
TextureHandle TextureCreate( const RmtImgui::CmdTexture* pCmdTexture )
{
	// Find a free handle
	TextureHandle texHandle;
	for(UINT i=0; !texHandle.IsValid() && i<gTextures.size(); ++i)
		if( gTextures[i].mBuffer.Get() == nullptr )
			texHandle.mIndex = i;
	
	if( !texHandle.IsValid() )
	{
		texHandle.mIndex = gTextures.size();
		gTextures.resize(texHandle.mIndex+1);
	}

	texHandle.mImguiId	= pCmdTexture->mTextureId;
	bool bSuccess		= CreateTexture(gTextures[texHandle.mIndex], pCmdTexture->mFormat, pCmdTexture->mWidth, pCmdTexture->mHeight, pCmdTexture->mpTextureData);	
	return bSuccess ? texHandle : TextureHandle();
}

void TextureRelease(const TextureHandle& hTexture)
{
	gTextures[hTexture.mIndex].mView.Release();
	gTextures[hTexture.mIndex].mBuffer.Release();
	
}

}
