#include "Defines.hlsl"

Texture2D<float4> Textures[] : register(t2) {};

SamplerState s0 : register(s0);

// Register (b0)
#include "CameraData.hlsl"

struct PS_in
{
    float4	Position	: POS_OUT;
    float2	UVs			: TEXCOORD0;
    uint 	TextureID 	: POSITION;
	float4	Color		: COLOR;
};

float4 main(PS_in IN) : TAR_OUT
{
#ifndef __PSSL__
	float4 outColor = Textures[IN.TextureID].Sample(s0, IN.UVs) * IN.Color;
	if (outColor.a == 0.0) discard;
	return outColor;
#else
	return float4(0.0, 0.0, 0.0, 1.0);
#endif
}