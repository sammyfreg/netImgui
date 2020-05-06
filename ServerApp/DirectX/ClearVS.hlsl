/*struct VS_INPUT
{
	float2 pos : POSITION;
	float2 uv  : TEXCOORD0;
};
*/
struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
};
   
PS_INPUT main(uint VtxId : SV_VertexID)
{
	PS_INPUT Vertice	= (PS_INPUT)0;	
	Vertice.pos.x		= VtxId & 0x01 ? 1 : -1;
	Vertice.pos.y		= VtxId & 0x02 ? 1 : -1;
	Vertice.pos.z		= 0.5;
	Vertice.pos.w		= 1;
	Vertice.uv			= Vertice.pos.xy * 0.5 + 0.5; // Rescale [-1,1] to [0,1]
	return Vertice;
}
