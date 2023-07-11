#include "Defines.hlsl"

CONSTANT_BUFFER Data : register(b0)
{
    matrix mvp;
    matrix mv;
	
    float4 camPosition;
    float4 camOffset;
	
	float4 viewport;
    float4 lightDirection;
	
    uint4 worldSize;
	
	uint sdfCount;
	uint particleCount;

    uint lightCount;
    float padding;
};

struct VS_out
{
	float2 UVs			: TEXCOORD0;
	float4 Position		: POS_OUT;
	float4 Direction 	: POSITION;
};

#include "Camera.hlsl"

VS_out main(uint uiID : VERT_ID)
{
	VS_out OUT;
	
    OUT.UVs = float2((uiID << 1) & 2, uiID & 2);
    OUT.Position = float4(OUT.UVs.x * 2.0 - 1.0, -OUT.UVs.y * 2.0 + 1.0, 0.0, 1.0);
	OUT.Direction = float4(GetRay(mv, OUT.UVs * viewport.xy, viewport.xy, viewport.z, viewport.w), 0.0);
	
	return OUT;
}