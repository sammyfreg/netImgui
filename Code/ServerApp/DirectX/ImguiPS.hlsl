 struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

 cbuffer pixelBuffer : register(b0)
{
	uint4 texFormat;
};

sampler sampler0;
Texture2D texture0;

// Must match 'NetImgui_Api.h'
static const uint kTexFmtA8		= 0;
static const uint kTexFmtRGBA8	= 1;

float4 main(PS_INPUT input) : SV_Target
{
	float4 texColor = texture0.Sample(sampler0, input.uv);

	if( texFormat.x == kTexFmtA8 )
		texColor = float4(1,1,1, texColor.r);

	float4 out_col	= input.col * texColor;
	return out_col;
}
