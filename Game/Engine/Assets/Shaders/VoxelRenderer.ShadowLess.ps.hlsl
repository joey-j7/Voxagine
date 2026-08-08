#include "Defines.hlsl"
#include "CameraData.hlsl" // register(b0)

SamplerState s0 : register(s0);

VOXEL_RW_BUFFER voxelWorldData : register(u0);

/* Occupancy counts per BRICK_SIZE^3 block - see SDFMarcher.hlsl. */
RW_STRUCTURED_BUFFER(uint) voxelBrickData : register(u1);

Texture2D<float4> particlePass : register(t1);
VOXEL_BUFFER voxelModelData[] : register(t2) {};

struct PS_in
{
    float4 NormScreenPosition	: POS_OUT;
	float4 Direction			: POSITION0;
    float3 WorldPosition		: POSITION1;
};

#include "SDFMarcher.hlsl"
#include "AmbientOcclusion.hlsl"

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

    /* Window diagonal in bricks - see VoxelRenderer.ps.hlsl for why the
       hardcoded 700 is gone. */
    int maxBrickSteps = int(length(float3(worldSize.xyz)) * BRICK_INV_SIZE) + 2;

    MarchResult marchDiffuse = MarchDiffuse
    (
		rayOrigin,
		rayDirection,
		maxBrickSteps
	);
	
    /* Return transparent when not marched against anything. Sky and endless
       ground are composited in PostProcessing.ps.hlsl instead of here - see
       VoxelRenderer.ps.hlsl. */
	if (marchDiffuse.Color.a == 0.0)
	{
		return float4(0.0, 0.0, 0.0, 0.0);
	}

    /* Directional lighting */
    float difference = clamp(dot(marchDiffuse.Normal, -lightDirection.xyz), 0.0, 1.0);
    float shadowMultiplier = difference * (1.0-AMBIENT_VALUE) + AMBIENT_VALUE;

    /* Fake specular "shine line" on lit voxel edges - see GetShineLine in
       AmbientOcclusion.hlsl. */
    shadowMultiplier *= GetShineLine(marchDiffuse.Position, marchDiffuse.Normal, marchDiffuse.UV, lightDirection.xyz, difference);

    /* Ambient occlusion - hit-time only, zero added per-step cost */
    float4 ambient = GetAmbientOcclusion(marchDiffuse.Position, marchDiffuse.Mask, marchDiffuse.SRDirection, marchDiffuse.Normal, marchDiffuse.UV);

    marchDiffuse.Color.xyz *= float3(shadowMultiplier, shadowMultiplier, shadowMultiplier) * ambient.xyz;
    marchDiffuse.Color.a = 1.0;

    return marchDiffuse.Color;
}