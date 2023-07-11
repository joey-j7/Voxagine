#include "Defines.hlsl"
#include "CameraData.hlsl" // register(b0)

SamplerState s0;

VOXEL_RW_BUFFER voxelWorldData : register(u0);

Texture2D<float4> particlePass : register(t1);
VOXEL_BUFFER voxelModelData[] : register(t2) {};

struct PS_in
{
    float4 NormScreenPosition	: POS_OUT;
	float4 Direction			: POSITION0;
    float3 WorldPosition		: POSITION1;
};

#include "SDFMarcher.hlsl"

FORCE_DEPTH_TEST
float4 main(PS_in IN) : TAR_OUT
{
    /* Check particle color */
    float4 particleColor = particlePass.Sample(s0, IN.NormScreenPosition.xy / (viewport.xy * voxelRenderScale));

    if (particleColor.a != 0.0)
        return particleColor;

    /* March diffuse color */
	float3 rayOrigin = IN.WorldPosition - camOffset.xyz;
	float3 rayDirection = normalize(IN.Direction.xyz);

    MarchResult marchDiffuse = MarchDiffuse
    (
		rayOrigin,
		rayDirection,
		worldSize,
		700
	);
	
    /* Return black when not marched against anything */
	if (marchDiffuse.Color.a == 0.0)
	{
		return float4(0.0, 0.0, 0.0, 0.0);
	}
    
    /* Directional lighting */
    float difference = clamp(dot(marchDiffuse.Normal, -lightDirection.xyz), 0.0, 1.0);
    float shadowMultiplier = difference * (1.0-AMBIENT_VALUE) + AMBIENT_VALUE;

    marchDiffuse.Color.xyz *= float3(shadowMultiplier, shadowMultiplier, shadowMultiplier);
    marchDiffuse.Color.a = 1.0;

    return marchDiffuse.Color;
}