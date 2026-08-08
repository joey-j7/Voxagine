#include "Defines.hlsl"
#include "CameraData.hlsl" // register(b0)

SamplerState s0 : register(s0);

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
#include "AmbientOcclusion.hlsl"

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
	
    /* Return transparent when not marched against anything. Sky and endless
       ground are composited in PostProcessing.ps.hlsl instead of here - this
       pass only rasterizes AABB proxy cubes (see RENDERING_PLAN.md phase 1
       notes), so it never covers the whole screen the way a full-screen sky
       needs. */
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
		 	shadowMultiplier = min(marchLighting.Distance * SHADOW_FADE_K * difference + AMBIENT_VALUE, 1.0);
		}
    }

    /* Fake specular "shine line" on lit voxel edges - not real specular, just
       a brightness kick near the top/front UV edge of a face that has no
       neighbour occluding it. Pulled forward from RENDERING_PLAN.md phase 5
       step 6 since it only needs res.UV/res.Normal, which this phase's AO
       work already populates. Ported from Splody
       VoxelRendererForward.ps.hlsl:300-303. */
    float3 shineTestPos = marchDiffuse.Position;
    if (marchDiffuse.Normal.z < -0.5 && marchDiffuse.UV.y >= 0.9 && !IsVoxel(shineTestPos + float3(0.0, 1.0, 0.0)))
        shadowMultiplier *= 1.02 * (1.0 + max(0.0, marchDiffuse.UV.y - 0.9) * 4.0);
    else if (marchDiffuse.Normal.y > 0.5 && marchDiffuse.UV.y <= 0.1 && !IsVoxel(shineTestPos + float3(0.0, 0.0, -1.0)))
        shadowMultiplier *= 1.02 * (1.0 + max(0.0, 0.1 - marchDiffuse.UV.y) * 4.0);

    /* Ambient occlusion - hit-time only, zero added per-step cost */
    float4 ambient = GetAmbientOcclusion(marchDiffuse.Position, marchDiffuse.Mask, marchDiffuse.SRDirection, marchDiffuse.Normal, marchDiffuse.UV);

    marchDiffuse.Color.xyz *= float3(shadowMultiplier, shadowMultiplier, shadowMultiplier) * ambient.xyz;
    marchDiffuse.Color.a = 1.0;

    return marchDiffuse.Color;
}