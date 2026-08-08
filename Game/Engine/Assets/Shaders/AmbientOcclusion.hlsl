bool IsVoxel(float3 position) {
	if (position.x < 0.0 || position.y < 0.0 || position.z < 0.0)
		return false;

	uint ID = PosToVoxelID(uint3(position));
	
#ifdef __PSSL__
	return (0xFF & (voxelWorldData[ID] >> 24)) > 0.0;
#else
	return voxelWorldData[ID].a > 0.0;
#endif
}

float GetVertexAO(float2 side, float fCorner) {
	if (side.x == 1.0 && side.y == 1.0) return 1.0;
	return (side.x + side.y + max(fCorner, side.x * side.y)) / 3.0;
}

float4 GetVoxelAO(float3 pos, float3 d1, float3 d2) {
	float4 side = float4(IsVoxel(pos + d1), IsVoxel(pos + d2), IsVoxel(pos - d1), IsVoxel(pos - d2));
	float4 corner = float4(IsVoxel(pos + d1 + d2), IsVoxel(pos - d1 + d2), IsVoxel(pos - d1 - d2), IsVoxel(pos + d1 - d2));
	
	float4 ao;
	
	ao.x = GetVertexAO(side.xy, corner.x);
	ao.y = GetVertexAO(side.yz, corner.y);
	ao.z = GetVertexAO(side.zw, corner.z);
	ao.w = GetVertexAO(side.wx, corner.w);
	
	return 1.0 - ao;
}

float4 GetAmbientOcclusion(float3 position, float3 mask, float3 srDirection, float3 normal, float2 uv) {
	float4 ambient = GetVoxelAO(position - mask * srDirection, mask.zxy, mask.yzx);

	if (abs(normal.b) > 0.5)
		uv = float2(uv.y, uv.x);

	float interpolatedAO = lerp(lerp(ambient.z, ambient.w, uv.y), lerp(ambient.y, ambient.x, uv.y), uv.x);
	interpolatedAO = pow(interpolatedAO, 1.0 / 3.0);

	float color = 0.75 + interpolatedAO * 0.25;

	return float4(pow(float3(color, color, color), float3(2.2, 2.2, 2.2)), 1);
}

/* Fake specular "shine line" - a brightness kick on the rim of a lit face
   that's open to the sky, simulating a grazing highlight. Not real specular.
   Follows lightDirection rather than being pinned to one hardcoded face
   orientation, unlike the Splody original this was ported from
   (VoxelRendererForward.ps.hlsl:300-303), which only ever lit a -Z-facing
   wall and a floor's -Z edge regardless of where the light actually was. */
float GetShineLine(float3 position, float3 normal, float2 uv, float3 lightDirection, float difference) {
	if (difference <= 0.1)
		return 1.0;

	/* Scales the highlight by how directly this face catches the light.
	   Without this, any face just past the difference > 0.1 lit threshold
	   got the same full boost as one facing the light head-on - for a wall,
	   `difference` is dot(normal, -lightDirection) with a purely horizontal
	   normal, so it already measures horizontal alignment with the light;
	   without scaling by it, walls facing away from the light got the same
	   rim boost as walls facing toward it, which read as a highlight fixed
	   to the model's own geometry rather than one that tracks the light. */
	float fIntensity = saturate((difference - 0.1) / 0.9);

	if (abs(normal.y) < 0.5) {
		/* Vertical wall: UV.y is always world-up (SDFMarcher.hlsl's UV
		   swap makes this true for every wall axis), so the sky-facing rim
		   is always the top edge, whichever way the wall itself faces. */
		if (uv.y >= 0.9 && !IsVoxel(position + float3(0.0, 1.0, 0.0)))
			return lerp(1.0, 1.02 * (1.0 + max(0.0, uv.y - 0.9) * 4.0), fIntensity);

		return 1.0;
	}

	if (normal.y > 0.5) {
		/* Floor: UV maps to (world X, world Z) with no swap. An angled light
		   faces partly toward both the X edge and the Z edge at once (ours
		   is roughly a 34/56 split), so evaluate both instead of snapping to
		   whichever axis dominates - a corner voxel under a diagonal light
		   should show a rim on two sides, not just the single
		   most-aligned edge. Each edge's boost is weighted by how much that
		   axis actually points toward the light, so a near-axis-aligned
		   light still fades the weaker edge out to nearly nothing. */
		float2 towardLight = -normalize(lightDirection.xz);

		float xBoost = 0.0;
		float3 xEdgeOffset = float3(sign(towardLight.x), 0.0, 0.0);
		float xEdgeUV = (towardLight.x > 0.0) ? uv.x : (1.0 - uv.x);
		if (xEdgeUV >= 0.9 && !IsVoxel(position + xEdgeOffset))
			xBoost = (1.02 * (1.0 + max(0.0, xEdgeUV - 0.9) * 4.0) - 1.0) * abs(towardLight.x);

		float zBoost = 0.0;
		float3 zEdgeOffset = float3(0.0, 0.0, sign(towardLight.y));
		float zEdgeUV = (towardLight.y > 0.0) ? uv.y : (1.0 - uv.y);
		if (zEdgeUV >= 0.9 && !IsVoxel(position + zEdgeOffset))
			zBoost = (1.02 * (1.0 + max(0.0, zEdgeUV - 0.9) * 4.0) - 1.0) * abs(towardLight.y);

		return lerp(1.0, 1.0 + max(xBoost, zBoost), fIntensity);
	}

	return 1.0;
}