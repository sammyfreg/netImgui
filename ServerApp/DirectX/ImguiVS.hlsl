 cbuffer vertexBuffer : register(b0)
{
	float4x4 ProjectionMatrix;
};

static const float kUvRange_Min		= -2;
static const float kUvRange_Max		= 2;
static const float kPosRange_Min	= -256;
static const float kPosRange_Max	= 2048;
struct VS_INPUT
{
	float2 pos : POSITION;
	float2 uv  : TEXCOORD0;
	float4 col : COLOR0;	
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};
           
PS_INPUT main(VS_INPUT input)
{
	PS_INPUT output		= (PS_INPUT)0;
	float2 VtxPos		= float2(input.pos.xy * (kPosRange_Max - kPosRange_Min) + kPosRange_Min);
	output.pos			= mul(ProjectionMatrix, float4(VtxPos,0,1));
	output.col			= input.col;
	output.uv			= float2(input.uv * (kUvRange_Max - kUvRange_Min) + kUvRange_Min);
	return output;
}