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

/*CONSTANT_BUFFER DataPart : register(b1)
{
    float RenderScale;
}*/

Texture2D<float> depthPass : register(t1);
SamplerState s0 : register(s0);

struct PS_in
{
    float4 Color		: COLOR;
    float4 PosVert      : POSITION;
    float4 Position		: POS_OUT;
};

#ifdef __PSSL__
[FORCE_EARLY_DEPTH_STENCIL]
#else
[earlydepthstencil]
#endif
float4 main(PS_in IN) : TAR_OUT
{
    float dist = distance(IN.PosVert.xyz, camPosition.xyz);
    float depthdist = depthPass.Sample(s0, IN.Position.xy * 2.0 / viewport.xy);

    if (dist >= depthdist)
    {
        return float4(0.0, 0.0, 0.0, 0.0);
    }
    
    return IN.Color;
}