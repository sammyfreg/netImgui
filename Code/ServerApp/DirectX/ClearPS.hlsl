 struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
};

float4 ClearColor;
float4 TextureTint;

sampler sampler0;
Texture2D texture0;
            
float4 main(PS_INPUT input) : SV_Target
{
	float4 outCol	= ClearColor;
	float4 texCol	= texture0.Sample(sampler0, input.uv) * TextureTint;
	outCol.rgb		= lerp(ClearColor.rgb, texCol.rgb, texCol.a);
	return outCol;
}
