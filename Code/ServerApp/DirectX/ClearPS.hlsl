 struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
};

float4 ClearColor;
float4 TextureTint;
float4 UVScaleAndOffset;

sampler sampler0;
Texture2D texture0;
            
float4 main(PS_INPUT input) : SV_Target
{
	static const float2 UVScale = UVScaleAndOffset.xy;
	static const float2 UVOffset = UVScaleAndOffset.zw;

	float4 outCol	= ClearColor;
	float4 texCol	= texture0.Sample(sampler0, UVOffset + input.uv*UVScale) * TextureTint;
	outCol.rgb		= lerp(ClearColor.rgb, texCol.rgb, texCol.a);
	return outCol;
}
