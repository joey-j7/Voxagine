#include "Defines.hlsl"
#include "CameraData.hlsl" // register(b0)

SamplerState s0;

VOXEL_RW_BUFFER voxelWorldData : register(u0);

Texture2D<float4> particlePass : register(t1);
Texture2D<float> particleDepthPass : register(t2);
VOXEL_BUFFER voxelModelData[] : register(t3) {};

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
    float2 particleUV = IN.NormScreenPosition.xy / (viewport.xy * voxelRenderScale);
    float4 particleColor = particlePass.Sample(s0, particleUV);
    float particleDepth = particleDepthPass.Sample(s0, particleUV);

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

    if (particleDepth < distance(marchDiffuse.SmoothPosition + camOffset, camPosition.xyz) && particleColor.a != 0.0)
    {
        return particleColor;
    }
	
    /* Return black when not marched against anything */
    if (marchDiffuse.Color.a == 0.0)
    {
        return float4(0.0, 0.0, 0.0, 0.0);
    }
    
    /* Directional lighting */
    float difference = clamp(dot(marchDiffuse.Normal, -lightDirection.xyz), 0.0, 1.0);
    float shadowMultiplier = difference * (1.0-AMBIENT_VALUE) + AMBIENT_VALUE;

    if (difference > 0.1)
	{
        MarchResult marchLighting = MarchLight(
            marchDiffuse.SmoothPosition - lightDirection.xyz,
            -lightDirection.xyz,
            float3(
                float(worldSize.x),
                float(worldSize.y),
                float(worldSize.z)
            ),
            64
        );

        if (marchLighting.Color.a > 0.0) {
		 	shadowMultiplier = AMBIENT_VALUE;
		}
    }

    marchDiffuse.Color.xyz *= float3(shadowMultiplier, shadowMultiplier, shadowMultiplier);
    marchDiffuse.Color.a = 1.0;

    return marchDiffuse.Color;
}