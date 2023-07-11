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