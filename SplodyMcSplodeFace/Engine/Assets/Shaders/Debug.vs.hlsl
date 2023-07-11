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

struct DebugLine
{
	float4 Position;
	float4 Color;
};

STRUCTURED_BUFFER(DebugLine) DebugLines : register(t0);

struct VS_out
{
    float4 Position		: POS_OUT;
    float4 Color		: COLOR;
};

VS_out main(uint IDvert : VERT_ID, uint IDinst : INST_ID)
{
	VS_out OUT;
	
	DebugLine debugLine = DebugLines[IDinst * 2 + IDvert];
	OUT.Position = mul(mvp, float4(debugLine.Position.xyz, 1.0));
	OUT.Color = debugLine.Color;
	
	return OUT;
}