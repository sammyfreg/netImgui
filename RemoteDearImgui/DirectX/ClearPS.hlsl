 struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
};

float4 ClearColor;

sampler sampler0;
Texture2D texture0;
            
float4 main(PS_INPUT input) : SV_Target
{
	float4 out_col	= float4(1,1,1,1);
	out_col.rgb		= lerp(texture0.Sample(sampler0, input.uv).rgb, ClearColor.rgb, ClearColor.a);
	return out_col;
}
