#include "Defines.hlsl"
#include "CameraData.hlsl" // register(b0)

SamplerState s0 : register(s0);

VOXEL_RW_BUFFER voxelWorldData : register(u0);

/* Occupancy counts per BRICK_SIZE^3 block - see SDFMarcher.hlsl. Declared
   read-write because the pass binds it as a read-write mapper, which is what
   keeps it out of the t register range the textures below already use. */
RW_STRUCTURED_BUFFER(uint) voxelBrickData : register(u1);

Texture2D<float4> particlePass : register(t1);
Texture2D<float> particleDepthPass : register(t2);

VOXEL_BUFFER voxelModelData[] : register(t4) {};

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

    /* Budget the walk by the window's diagonal in bricks, so nothing distant
       is truncated. This replaces the hardcoded 700-voxel cap the scale-up
       introduced - on a 768x128x768 window the diagonal is ~1094 voxels, so
       700 cut every long sightline short. The brick walk is what makes the
       full diagonal affordable (RENDERING_PLAN.md phase 2). */
    int maxBrickSteps = int(length(float3(worldSize.xyz)) * BRICK_INV_SIZE) + 2;

    int primaryBrickSteps = maxBrickSteps;

    MarchResult marchDiffuse = MarchDiffuse
    (
		rayOrigin,
		rayDirection,
		primaryBrickSteps
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
        /* Same budget as the primary ray, and at full stride. The 64-step,
           2x-stride walk this replaces could not reach across the level and
           stepped straight over single-voxel-thick occluders; its coarse
           quantization is also what made phase 1's distance fade band. */
        MarchResult marchLighting = MarchLight(
            marchDiffuse.SmoothPosition - lightDirection.xyz,
            -lightDirection.xyz,
            maxBrickSteps
        );

        if (marchLighting.Color.a > 0.0) {
		 	shadowMultiplier = min(marchLighting.Distance * SHADOW_FADE_K * difference + AMBIENT_VALUE, 1.0);
		}
    }

    /* Fake specular "shine line" on lit voxel edges - see GetShineLine in
       AmbientOcclusion.hlsl. Pulled forward from RENDERING_PLAN.md phase 5
       step 6 since it only needs res.UV/res.Normal, which this phase's AO
       work already populates. */
    shadowMultiplier *= GetShineLine(marchDiffuse.Position, marchDiffuse.Normal, marchDiffuse.UV, lightDirection.xyz, difference);

    /* Ambient occlusion - hit-time only, zero added per-step cost */
    float4 ambient = GetAmbientOcclusion(marchDiffuse.Position, marchDiffuse.Mask, marchDiffuse.SRDirection, marchDiffuse.Normal, marchDiffuse.UV);

    marchDiffuse.Color.xyz *= float3(shadowMultiplier, shadowMultiplier, shadowMultiplier) * ambient.xyz;
    marchDiffuse.Color.a = 1.0;

    return marchDiffuse.Color;
}