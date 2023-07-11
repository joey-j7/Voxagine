#include "Defines.hlsl"
#include "CameraData.hlsl" // register(b0)

/*CONSTANT_BUFFER DataPart : register(b1)
{
    float RenderScale;
}*/

// Texture2D<float> depthPass : register(t1);
SamplerState s0 : register(s0);

struct PS_in
{
    float4 Color		: COLOR;
    float4 PosVert      : POSITION;
    float4 Position		: POS_OUT;
};

struct PS_out
{
    float4 Color : TAR_OUT0;
    float Depth : TAR_OUT1;
};

FORCE_DEPTH_TEST
PS_out main(PS_in IN)
{
    // float dist = distance(IN.PosVert.xyz, camPosition.xyz);

    // float depthdist = depthPass.Sample(s0, IN.Position.xy * 2.0 / viewport.xy);

    // if (dist >= depthdist)
    // {
    //     return float4(0.0, 0.0, 0.0, 0.0);
    // }

    PS_out OUT;
    
    OUT.Color = IN.Color;
    OUT.Depth = distance(IN.PosVert.xyz, camPosition.xyz);

    return OUT;
}