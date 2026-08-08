/* Consumer side of the low-resolution depth prepass - RENDERING_PLAN.md
   phase 3. Include after declaring:

     Texture2D<float> prepassTexture;

   SDFPrepass.ps.hlsl writes, per texel, the distance from the camera at which
   that texel's camera ray first enters an occupied brick. Both voxel pixel
   shaders start their march there instead of at the proxy cube's face, so the
   run of empty bricks in front of the geometry is walked once per texel rather
   than once per pixel - a 1/PREPASS_SCALE^2 reduction in the part of the march
   that is pure empty space, which is most of it at any long sightline. */

/* Nearest brick entry over the 3x3 prepass texels around a pixel, in world
   units from the camera; PREPASS_MISS when none of them found anything.

   The minimum is the whole safety argument. One texel's ray is not the pixel's
   ray, and hit distance across a texel footprint can vary by the length of the
   level at a grazing angle - so taking the nearest of the neighbourhood, then
   subtracting PREPASS_MARGIN, is what keeps the skip behind anything the pixel
   itself could hit. It is not a proof: geometry thinner than the footprint can
   pass between all nine rays, which is why the prepass measures brick entry
   rather than the voxel hit (see SDFPrepass.ps.hlsl). */
float GetPrepassEntry(Texture2D<float> prepass, float2 v2PixelPosition)
{
	/* Texel coordinates rather than a sampler: the engine's samplers repeat,
	   and a neighbour offset that wraps to the far edge of the screen would
	   report a distance from somewhere else entirely - which, being a minimum,
	   could only ever be too small in the direction that skips too far. */
	int2 v2Size = int2(max(floor(viewport.xy * voxelRenderScale * PREPASS_SCALE), float2(1.0, 1.0)));
	int2 v2Texel = int2(v2PixelPosition * PREPASS_SCALE);

	float fNearest = PREPASS_MISS;

	[unroll]
	for (int y = -1; y <= 1; y++)
	{
		[unroll]
		for (int x = -1; x <= 1; x++)
		{
			int2 v2Sample = clamp(v2Texel + int2(x, y), int2(0, 0), v2Size - 1);
			fNearest = min(fNearest, prepass.Load(int3(v2Sample, 0)));
		}
	}

	return fNearest;
}

/* How far along the ray a pixel may start marching. v3Direction is the
   *unnormalized* vector from the camera to the fragment, so its length is
   where this fragment already sits along its own camera ray - the prepass
   measures from the camera, the march starts at the proxy cube's face, and the
   difference between the two is what is left to skip. */
float GetPrepassSkip(Texture2D<float> prepass, float2 v2PixelPosition, float3 v3Direction)
{
#if PREPASS_ENABLED
	float fEntry = GetPrepassEntry(prepass, v2PixelPosition);

	/* Written so that a miss - and a NaN, which compares false against
	   everything - falls through to no skip at all. */
	if (fEntry < PREPASS_MISS_TEST)
		return max(0.0, fEntry - PREPASS_MARGIN - length(v3Direction));
#endif

	return 0.0;
}
