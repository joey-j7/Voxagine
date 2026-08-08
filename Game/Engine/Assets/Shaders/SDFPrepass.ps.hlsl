#include "Defines.hlsl"
#include "CameraData.hlsl" // register(b0)

/* Declared, but only voxelBrickData is read - the prepass never descends to
   voxel level. voxelWorldData is here because SDFMarcher.hlsl's shared helpers
   name it, and because keeping it at u0 is what puts the brick counts at u1,
   the same pair VoxelPass binds. DXC drops the unused declaration. */
VOXEL_RW_BUFFER voxelWorldData : register(u0);
RW_STRUCTURED_BUFFER(uint) voxelBrickData : register(u1);

#include "SDFMarcher.hlsl"

/* Low-resolution depth prepass - RENDERING_PLAN.md phase 3.
 *
 * Writes, per texel, the distance from the camera at which that texel's camera
 * ray first enters an occupied brick, or PREPASS_MISS when it never does.
 * VoxelRenderer.ps.hlsl takes the minimum over a 3x3 neighbourhood of this and
 * starts its own march there, so the empty prefix of the ray gets walked once
 * per texel rather than once per pixel.
 *
 * Brick entry rather than the exact voxel hit, deliberately. It is the cheaper
 * measurement (no fine DDA at all) and the safer one: geometry thinner than a
 * texel footprint can slip between the nine sampled rays and be skipped past,
 * and a brick is BRICK_SIZE voxels wide, so a one-voxel feature is caught by
 * the neighbourhood at BRICK_SIZE times the distance a voxel-exact prepass
 * would catch it. It gives up almost nothing: the fine march has to walk the
 * occupied bricks either way, and what is skipped is exactly the run of empty
 * ones in front of them. */
float main(float4 position : POS_OUT) : TAR_OUT
{
	/* Orthographic. VoxelRenderer.vs.hlsl builds its ray as
	   (worldPosition - camPosition), which is the camera ray only under
	   perspective; under an orthographic projection those rays converge on the
	   camera and the consumer's "distance along the ray from the camera" has no
	   meaning. Report a miss so it skips nothing. */
	if (viewport.z == 0.0)
		return PREPASS_MISS;

	/* This target's own size. viewport.xy is the render resolution and
	   voxelRenderScale the scale the voxel pass renders at, so the prepass is
	   PREPASS_SCALE of that - see DepthPrepass::k_fScale. */
	float2 v2Size = max(floor(viewport.xy * voxelRenderScale * PREPASS_SCALE), float2(1.0, 1.0));

	/* Clip space of the very matrix VoxelRenderer.vs.hlsl projects its proxy
	   cubes with, so the ray reconstructed here is the one the pixels under
	   this texel will march along - not an FOV-and-aspect rebuild of it that
	   can drift by a fraction of a degree toward the screen edges. Y is flipped
	   because Draw uses a negative-height viewport (see VKRenderPass).

	   Any clip z on the ray gives the same direction, so 0.5 is used rather
	   than a near/far pair: it is inside the valid range under both the
	   [-1, 1] and [0, 1] depth conventions, and nothing here needs to know
	   which one GLM built the projection with. */
	float2 v2NDC = float2(2.0 * position.x / v2Size.x - 1.0, 1.0 - 2.0 * position.y / v2Size.y);

	float4 v4World = mul(invMvp, float4(v2NDC, 0.5, 1.0));

	if (v4World.w == 0.0)
		return PREPASS_MISS;

	float3 v3Direction = v4World.xyz / v4World.w - camPosition.xyz;
	float fDirectionLength = length(v3Direction);

	if (!(fDirectionLength > 0.0))
		return PREPASS_MISS;

	v3Direction /= fDirectionLength;

	/* Marcher space is world space minus the chunk window's offset, the same
	   space VoxelRenderer.ps.hlsl marches in. */
	float3 v3Origin = camPosition.xyz - camOffset.xyz;
	float fEntry = 0.0;

	if (!IsInChunk(int3(floor(v3Origin))))
	{
		float fToWorld = GetDistanceToWorld(v3Origin, 1.0 / v3Direction, float3(worldSize.xyz));

		if (fToWorld < 0.0)
			return PREPASS_MISS;

		fEntry = fToWorld;

		/* The entry point lands exactly on a face of the window, where floor()
		   can name the cell on the far side. Nudging inside costs less than the
		   margin the consumer already subtracts. */
		v3Origin = clamp(
			v3Origin + v3Direction * fEntry,
			float3(WORLD_ENTRY_EPSILON, WORLD_ENTRY_EPSILON, WORLD_ENTRY_EPSILON),
			float3(worldSize.xyz) - WORLD_ENTRY_EPSILON
		);

		if (!IsInChunk(int3(floor(v3Origin))))
			return PREPASS_MISS;
	}

	float fBrickEntry = MarchBrickEntry(
		v3Origin,
		v3Direction,
		int(length(float3(worldSize.xyz)) * BRICK_INV_SIZE) + 2
	);

	if (fBrickEntry < 0.0)
		return PREPASS_MISS;

	return fEntry + fBrickEntry;
}
