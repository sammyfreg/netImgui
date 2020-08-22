#include "stdafx.h"
#include <mutex>
#include <vector>
#include <cstddef>
#include <d3d11.h>
#include "DirectX11.h"
#include <ClearVS.h>
#include <ClearPS.h>
#include <ImguiVS.h>
#include <ImguiPS.h>
#include <Private/NetImgui_CmdPackets.h>

//-----------------------------------------------------------------------------
// Include image loader library, after turning off some warning that are triggered by it (in -wall)
#define STB_IMAGE_IMPLEMENTATION
#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wcast-qual"
	#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
	#pragma clang diagnostic ignored "-Wreserved-id-macro"
	#pragma clang diagnostic ignored "-Wcast-align"
	#pragma clang diagnostic ignored "-Wmissing-variable-declarations"
	#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
	#pragma clang diagnostic ignored "-Wextra-semi-stmt"
	#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
	#pragma clang diagnostic ignored "-Wold-style-cast"
	#pragma clang diagnostic ignored "-Wsign-conversion"
	#pragma clang diagnostic ignored "-Wdouble-promotion"
	#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
	#include "../ThirdParty/stb_image.h"
	#pragma clang diagnostic pop
#elif _MSC_VER
	#pragma warning (push)
	#pragma warning (disable: 4711)		// function 'xxx' selected for automatic inline expansion	
	#pragma warning (disable: 4296)		// '>=' : expression is always true
	#pragma warning (disable: 6011)		// Dereferencing NULL pointer 'y'. 
	#pragma warning (disable: 6262)		// Function uses '35012' bytes of stack:  exceeds /analyze:stacksize '16384'.  Consider moving some data to heap.	
	#pragma warning (disable: 6308)		// 'realloc' might return null pointer: assigning null pointer to 'out', which is passed as an argument to 'realloc', will cause the original memory block to be leaked.
	#pragma warning (disable: 6387)		// 'out+((((layers-1))*stride))' could be '0':  this does not adhere to the specification for the function 'memcpy'. 
	#pragma warning (disable: 26451)	// Arithmetic overflow: Using operator '+' on a 4 byte value and then casting the result to a 8 byte value. ...	
	#pragma warning (disable: 28182)	// Dereferencing NULL pointer. '*delays' contains the same NULL value as 'realloc()`6615' did. 
	#include "../ThirdParty/stb_image.h"
	#pragma warning (pop)	
#endif
#undef STB_IMAGE_IMPLEMENTATION
//-----------------------------------------------------------------------------

#pragma comment (lib, "D3D11.lib") 

namespace dx
{

template <typename DXRestype>
class TDXResource
{
public:
	TDXResource()						{}
	TDXResource(const TDXResource& copy){ mpResource = copy.mpResource; if(mpResource) mpResource->AddRef(); }
	~TDXResource()						{ Release(); }
	DXRestype** GetForInit()			{ Release(); return &mpResource; }
	DXRestype*	Get()					{ return mpResource; }
	DXRestype**	GetArray()				{ return &mpResource; }
	DXRestype*	operator->()			{ return mpResource; }
	void Release()						{ if(mpResource) mpResource->Release(); mpResource = nullptr; }
protected:
	DXRestype* mpResource = nullptr;
};

template <typename DXBufferType, typename DXViewType>
struct TDXBufferViewRes
{	
	TDXBufferViewRes(){}
	TDXBufferViewRes(const TDXBufferViewRes& copy) : mBuffer(copy.mBuffer), mView(copy.mView){}
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
										GfxResources(){}
	HWND								mhWindow			= NULL;
	UINT								mFrameIndex			= 0;
	UINT								mScreenWidth		= 0;
	UINT								mScreenHeight		= 0;
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
	ResTexture2D						mTransparentTex;
	std::vector<ResTexture2D>			mTextures;
	std::mutex							mTextureLock;
private:
	GfxResources(const GfxResources&) noexcept {}
	GfxResources(const GfxResources&&) noexcept {}
	GfxResources& operator=(const GfxResources&)noexcept {return *this;}
	GfxResources& operator=(const GfxResources&&)noexcept{return *this;}
};

struct TextureUpdate 
{ 
	uint64_t mIndex									= 0; 
	NetImgui::Internal::CmdTexture* mpCmdTexture	= nullptr; 
};

static GfxResources*					gpGfxRes = nullptr;
static TextureUpdate					gTexturesPending[64];
static std::atomic_uint32_t				gTexturesPendingCount(0);
static std::atomic_uint32_t				gTexturesMaxCount(0);

bool CreateTexture(ResTexture2D& OutTexture, NetImgui::eTexFormat format, uint16_t width, uint16_t height, const uint8_t* pPixelData)
{
	// Create the texture buffer
	D3D11_TEXTURE2D_DESC texDesc;
	
    ZeroMemory(&texDesc, sizeof(texDesc));	
	DXGI_FORMAT texFmt(DXGI_FORMAT_UNKNOWN);
	switch( format )
	{
	case NetImgui::eTexFormat::kTexFmtA8:		texFmt = DXGI_FORMAT_R8_UNORM; break;
	case NetImgui::eTexFormat::kTexFmtRGBA8:	texFmt = DXGI_FORMAT_R8G8B8A8_UNORM; break;
	case NetImgui::eTexFormat::kTexFmt_Invalid:	texFmt = DXGI_FORMAT_UNKNOWN; break;
	}

	if( texFmt == DXGI_FORMAT_UNKNOWN )
		return false;

    texDesc.Width						= width;
    texDesc.Height						= height;
    texDesc.MipLevels					= 1;
    texDesc.ArraySize					= 1;
    texDesc.Format						= texFmt;
    texDesc.SampleDesc.Count			= 1;
    texDesc.Usage						= D3D11_USAGE_DEFAULT;
    texDesc.BindFlags					= D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags				= 0;
	D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem					= pPixelData;
    subResource.SysMemPitch				= static_cast<UINT>( GetTexture_BytePerLine(format, width) );
    subResource.SysMemSlicePitch		= 0;
    HRESULT Result						= gpGfxRes->mDevice->CreateTexture2D(&texDesc, &subResource, OutTexture.mBuffer.GetForInit());

    // Create texture view
	if( Result == S_OK )
	{    
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format						= texFmt;
		srvDesc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels			= texDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip	= 0;
		gpGfxRes->mDevice->CreateShaderResourceView(OutTexture.mBuffer.Get(), &srvDesc, OutTexture.mView.GetForInit());
	}   
	return Result == S_OK;
}

void CreateCBuffer(ResCBuffer& ConstantBuffer, const void* pData, UINT DataSize)
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

void CreateVtxBuffer(ResVtxBuffer& VertexBuffer, const void* pData, UINT DataSize)
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

void CreateIdxBuffer(ResIdxBuffer& IndexBuffer, const void* pData, UINT DataSize)
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

bool CreateShaderBinding(ResShaderRes& OutShaderBind, const BYTE* VS_Data, size_t VS_Size, const BYTE* PS_Data, size_t PS_Size, const D3D11_INPUT_ELEMENT_DESC* Layout, size_t LayoutCount )
{	
	HRESULT hr = gpGfxRes->mDevice->CreateVertexShader(VS_Data, static_cast<UINT>(VS_Size), nullptr, OutShaderBind.mShaderVS.GetForInit());
	if( hr != S_OK )
		return false;

	hr = gpGfxRes->mDevice->CreatePixelShader(PS_Data, static_cast<UINT>(PS_Size), nullptr, OutShaderBind.mShaderPS.GetForInit());
	if( hr != S_OK )
		return false;

	hr = gpGfxRes->mDevice->CreateInputLayout(Layout, static_cast<UINT>(LayoutCount), VS_Data, static_cast<UINT>(VS_Size), OutShaderBind.mInputLayout.GetForInit() );
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
	gpGfxRes										= new GfxResources;
	gpGfxRes->mhWindow								= hWindow;
	gpGfxRes->mFrameIndex							= 0;
	gpGfxRes->mScreenWidth							= static_cast<UINT>(rcClient.right - rcClient.left);
	gpGfxRes->mScreenHeight							= static_cast<UINT>(rcClient.bottom - rcClient.top);

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
		hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, NULL, featureLevelArray, 1,
			D3D11_SDK_VERSION, &SwapDesc, gpGfxRes->mSwapChain.GetForInit(), gpGfxRes->mDevice.GetForInit(), &featureLevel, gpGfxRes->mContext.GetForInit());
	}		

	//Create our BackBuffer and RT view	
	if( hr == S_OK )
	{
		TDXResource<ID3D11Texture2D> BackBuffer;	
		hr = gpGfxRes->mSwapChain->GetBuffer(0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>(BackBuffer.GetForInit()));
		if( hr == S_OK )
			hr = gpGfxRes->mDevice->CreateRenderTargetView(BackBuffer.Get(), nullptr, gpGfxRes->mBackbufferView.GetForInit());
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
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.ScissorEnable = true;
        desc.DepthClipEnable = true;
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
			{ "POSITION", 0, DXGI_FORMAT_R16G16_UNORM,	0, offsetof(NetImgui::Internal::ImguiVert, mPos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R16G16_UNORM,  0, offsetof(NetImgui::Internal::ImguiVert, mUV),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,0, offsetof(NetImgui::Internal::ImguiVert, mColor), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		if( !CreateShaderBinding( gpGfxRes->mImguiShaders, gpShader_ImguiVS, sizeof(gpShader_ImguiVS), gpShader_ImguiPS, sizeof(gpShader_ImguiPS), ImguiVtxLayout, NetImgui::Internal::ArrayCount(ImguiVtxLayout) ) )
			return false;
	}

	if( hr == S_OK )
	{
		int Width(0), Height(0), Channel(0);
		stbi_uc* pBGPixels = stbi_load("Background.png", &Width, &Height, &Channel, 0);
		if( pBGPixels )
		{		
			// @Sammyfreg TODO : Support multiple format for Background
			CreateTexture(gpGfxRes->mBackgroundTex, NetImgui::eTexFormat::kTexFmtRGBA8, uint16_t(Width), uint16_t(Height), pBGPixels);
			delete[] pBGPixels;
		}

		uint8_t TransparentPixels[8*8];
		memset(TransparentPixels, 0, sizeof(TransparentPixels));
		CreateTexture(gpGfxRes->mTransparentTex, NetImgui::eTexFormat::kTexFmtA8, 8, 8, TransparentPixels);		
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
	UINT ScreenWidth	= static_cast<UINT>(rcClient.right - rcClient.left);
	UINT ScreenHeight	= static_cast<UINT>(rcClient.bottom - rcClient.top);
	if( gpGfxRes->mScreenWidth != ScreenWidth || gpGfxRes->mScreenHeight != ScreenHeight )
	{
		gpGfxRes->mScreenWidth	= ScreenWidth;
		gpGfxRes->mScreenHeight	= ScreenHeight;
		gpGfxRes->mContext->ClearState();
		gpGfxRes->mBackbufferView.Release();
		HRESULT hr = gpGfxRes->mSwapChain->ResizeBuffers(0, gpGfxRes->mScreenWidth, gpGfxRes->mScreenHeight, DXGI_FORMAT_UNKNOWN, 0);
		if( hr == S_OK )
		{
			DXGI_SWAP_CHAIN_DESC sd;
			gpGfxRes->mSwapChain->GetDesc(&sd);
			D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
			ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
			render_target_view_desc.Format			= sd.BufferDesc.Format;
			render_target_view_desc.ViewDimension	= D3D11_RTV_DIMENSION_TEXTURE2D;

			TDXResource<ID3D11Texture2D> BackBuffer;	
			hr = gpGfxRes->mSwapChain->GetBuffer(0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>(BackBuffer.GetForInit()));
			if( hr == S_OK )
				gpGfxRes->mDevice->CreateRenderTargetView(BackBuffer.Get(), &render_target_view_desc, gpGfxRes->mBackbufferView.GetForInit());
		}
	}
}

void Render_Clear()
{
	CD3D11_VIEWPORT vp(0.f, 0.f, static_cast<float>(gpGfxRes->mScreenWidth), static_cast<float>(gpGfxRes->mScreenHeight));
	const D3D11_RECT r = { 0, 0, static_cast<long>(gpGfxRes->mScreenWidth), static_cast<long>(gpGfxRes->mScreenHeight)};
	const float BlendFactor[4] = { 1.f, 1.f, 1.f, 1.f };

	ResCBuffer ClearCB;
	struct ClearParam
	{
		ImVec4 mClearColor;
		ImVec4 mTextureTint; 
	} clearParam;
	clearParam.mClearColor	= ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	clearParam.mTextureTint = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
		
	CreateCBuffer(ClearCB, &clearParam, sizeof(clearParam));
	gpGfxRes->mContext->OMSetRenderTargets(1, gpGfxRes->mBackbufferView.GetArray(), nullptr );
	gpGfxRes->mContext->OMSetBlendState(gpGfxRes->mClearBlendState.Get(), BlendFactor, 0xFFFFFFFF);
	gpGfxRes->mContext->RSSetViewports(1, &vp);
	gpGfxRes->mContext->RSSetScissorRects(1, &r);
	gpGfxRes->mContext->RSSetState(gpGfxRes->mDefaultRasterState.Get());
	gpGfxRes->mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	gpGfxRes->mContext->IASetInputLayout(gpGfxRes->mClearShaders.mInputLayout.Get());
	gpGfxRes->mContext->VSSetShader(gpGfxRes->mClearShaders.mShaderVS.Get(), nullptr, 0);
	gpGfxRes->mContext->PSSetShader(gpGfxRes->mClearShaders.mShaderPS.Get(), nullptr, 0);
	gpGfxRes->mContext->PSSetConstantBuffers(0, 1, ClearCB.GetArray());
	gpGfxRes->mContext->PSSetSamplers(0, 1, gpGfxRes->mDefaultSampler.GetArray());
	gpGfxRes->mContext->PSSetShaderResources(0, 1, gpGfxRes->mBackgroundTex.mView.GetArray());
 	gpGfxRes->mContext->Draw(4,0);
}

void Render_DrawImgui(const std::vector<TextureHandle>& textures, const NetImgui::Internal::CmdDrawFrame* pDrawFrame)
{
	if( !pDrawFrame )
		return;
	
	const float L = pDrawFrame->mDisplayArea[0];
	const float R = pDrawFrame->mDisplayArea[2];	
	const float T = pDrawFrame->mDisplayArea[1];
	const float B = pDrawFrame->mDisplayArea[3];
	const float mvp[4][4] = 
	{
	    { 2.0f/(R-L),   0.0f,           0.0f,       0.0f},
	    { 0.0f,         2.0f/(T-B),     0.0f,       0.0f,},
	    { 0.0f,         0.0f,           0.5f,       0.0f },
	    { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
	};

	const float BlendFactor[4] = { 1.f, 1.f, 1.f, 1.f };
	const unsigned int stride = sizeof(NetImgui::Internal::ImguiVert);
	const unsigned int offset = 0;
	ResCBuffer VertexCB;
	ResCBuffer PixelCB;
	ResVtxBuffer VertexBuffer;
	ResVtxBuffer IndexBuffer;
	CreateCBuffer(VertexCB, reinterpret_cast<const void*>(mvp), sizeof(mvp));
	CreateVtxBuffer(VertexBuffer, pDrawFrame->mpVertices.Get(), pDrawFrame->mVerticeCount*sizeof(NetImgui::Internal::ImguiVert));
	CreateIdxBuffer(IndexBuffer, pDrawFrame->mpIndices.Get(), pDrawFrame->mIndiceByteSize);

	gpGfxRes->mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gpGfxRes->mContext->OMSetBlendState(gpGfxRes->mImguiBlendState.Get(), BlendFactor, 0xFFFFFFFF);
	gpGfxRes->mContext->IASetInputLayout(gpGfxRes->mImguiShaders.mInputLayout.Get());
	gpGfxRes->mContext->VSSetShader(gpGfxRes->mImguiShaders.mShaderVS.Get(), nullptr, 0);
	gpGfxRes->mContext->PSSetShader(gpGfxRes->mImguiShaders.mShaderPS.Get(), nullptr, 0);
	gpGfxRes->mContext->VSSetConstantBuffers(0, 1, VertexCB.GetArray());
	gpGfxRes->mContext->IASetVertexBuffers(0, 1, VertexBuffer.GetArray(), &stride, &offset);

	CD3D11_RECT				RectPrevious(0,0,0,0);
	uint64_t				lastTextureId	= NetImgui::Internal::u64Invalid;
	NetImgui::eTexFormat	eLastTexFmt		= NetImgui::eTexFormat::kTexFmt_Invalid;
	for(unsigned int i(0); i<pDrawFrame->mDrawCount; ++i)
	{
		const auto* pDraw = &pDrawFrame->mpDraws[i];
		CD3D11_RECT Rect(static_cast<LONG>(pDraw->mClipRect[0]), static_cast<LONG>(pDraw->mClipRect[1]), static_cast<LONG>(pDraw->mClipRect[2]), static_cast<LONG>(pDraw->mClipRect[3]) );
		if( i == 0 || pDraw->mIndexSize != pDrawFrame->mpDraws[i-1].mIndexSize )
		{			
			gpGfxRes->mContext->IASetIndexBuffer(IndexBuffer.Get(), pDraw->mIndexSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
		}

		if( i == 0 || Rect != RectPrevious )
		{
			RectPrevious = Rect;
			gpGfxRes->mContext->RSSetScissorRects(1, &RectPrevious);
		}

		if( i == 0 || lastTextureId != pDraw->mTextureId )
		{			
			int foundIdx = -1;
			for(size_t j(0); foundIdx < 0 && j<textures.size(); ++j)
			{
				if( textures[j].mImguiId == pDraw->mTextureId && 
					textures[j].mIndex < gpGfxRes->mTextures.size() &&
					gpGfxRes->mTextures[textures[j].mIndex].mBuffer.Get() != nullptr )
				{
					foundIdx = static_cast<int>(j);
				}
			}
						
			ResTexture2D* pTexture			= foundIdx < 0 ? &gpGfxRes->mTransparentTex			: &gpGfxRes->mTextures[textures[foundIdx].mIndex];	// Default texture if not found
			NetImgui::eTexFormat eTexFmt	= foundIdx < 0 ? NetImgui::eTexFormat::kTexFmtA8	: textures[foundIdx].mTextureFormat;
			if( eTexFmt != eLastTexFmt )
			{
				uint32_t pixelParams[4] = {static_cast<uint32_t>(eTexFmt),0,0,0};
				CreateCBuffer(PixelCB, reinterpret_cast<const void*>(pixelParams), sizeof(pixelParams));
				
			}
			gpGfxRes->mContext->PSSetShaderResources(0, 1, pTexture->mView.GetArray() );
			gpGfxRes->mContext->PSSetConstantBuffers(0, 1, PixelCB.GetArray());
			lastTextureId	= pDraw->mTextureId;
			eLastTexFmt		= eTexFmt;
		}

		gpGfxRes->mContext->DrawIndexed(pDraw->mIdxCount, pDraw->mIdxOffset/pDraw->mIndexSize, static_cast<INT>(pDraw->mVtxOffset));
	}
	gpGfxRes->mContext->PSSetShaderResources(0, 1, gpGfxRes->mTransparentTex.mView.GetArray());
}

void Render_UpdatePendingResources()
{
	if(	gpGfxRes->mTextures.size() < gTexturesMaxCount )
		gpGfxRes->mTextures.resize(gTexturesMaxCount);

	while( gTexturesPendingCount != 0 )
	{
		auto pendingIdx		= gTexturesPendingCount.fetch_sub(1)-1;
		auto texIdx			= gTexturesPending[pendingIdx].mIndex;
		auto pCmdTexture	= gTexturesPending[pendingIdx].mpCmdTexture;
		if( pCmdTexture == nullptr )
		{
			gpGfxRes->mTextures[texIdx].mView.Release();
			gpGfxRes->mTextures[texIdx].mBuffer.Release();
		}
		else
		{
			CreateTexture(gpGfxRes->mTextures[texIdx], static_cast<NetImgui::eTexFormat>(pCmdTexture->mFormat), pCmdTexture->mWidth, pCmdTexture->mHeight, pCmdTexture->mpTextureData.Get());
			netImguiDeleteSafe(pCmdTexture);
		}
		gTexturesPending[pendingIdx].mIndex			= NetImgui::Internal::u64Invalid;
		gTexturesPending[pendingIdx].mpCmdTexture	= nullptr;
	}
}

void Render(const std::vector<TextureHandle>& textures, const NetImgui::Internal::CmdDrawFrame* pDrawFrame)
{
	if( !gpGfxRes )
		return;
	
	Render_UpdatePendingResources();
	Render_UpdateWindowSize();
	Render_Clear();
	Render_DrawImgui(textures, pDrawFrame);
		
	ID3D11RenderTargetView* RenderTargetNone[4]={nullptr,nullptr,nullptr,nullptr};
	gpGfxRes->mContext->OMSetRenderTargets( static_cast<UINT>(NetImgui::Internal::ArrayCount(RenderTargetNone)), RenderTargetNone, nullptr );
	gpGfxRes->mSwapChain->Present(0, 0);		
	gpGfxRes->mFrameIndex++;
}

TextureHandle TextureCreate( NetImgui::Internal::CmdTexture* pCmdTexture )
{
	// Find a free handle
	TextureHandle texHandle;
	{		
		std::lock_guard<std::mutex> guard(gpGfxRes->mTextureLock);
		for(UINT i=0; !texHandle.IsValid() && i< gpGfxRes->mTextures.size(); ++i)
			if(gpGfxRes->mTextures[i].mBuffer.Get() == nullptr )
				texHandle.mIndex = i;
	}
	
	// Initialize the handle
	if( !texHandle.IsValid() )
		texHandle.mIndex = gTexturesMaxCount.fetch_add(1);		
	texHandle.mImguiId					= pCmdTexture->mTextureId;
	texHandle.mTextureFormat			= pCmdTexture->mFormat;

	// Add this texture to be added in main thread
	// @Sammyfreg TODO: create a threadsafe consume/append buffer
	while( gTexturesPendingCount == NetImgui::Internal::ArrayCount(gTexturesPending) )
		Sleep(0);
	auto idx							= gTexturesPendingCount.fetch_add(1);
	gTexturesPending[idx].mIndex		= texHandle.mIndex;
	gTexturesPending[idx].mpCmdTexture	= pCmdTexture;

	return texHandle;
}

void TextureRelease(TextureHandle& hTexture)
{
	if( hTexture.IsValid() )
	{
		while( gTexturesPendingCount == NetImgui::Internal::ArrayCount(gTexturesPending) )
			Sleep(0);

		auto idx							= gTexturesPendingCount.fetch_add(1);
		gTexturesPending[idx].mIndex		= hTexture.mIndex;
		gTexturesPending[idx].mpCmdTexture	= nullptr;
		hTexture.SetInvalid();
	}
}

}
