#include "Defines.hlsl"

CONSTANT_BUFFER Data : register(b0)
{
    matrix mvp;
    matrix mv;
	
    float4 camPosition;
    float4 camOffset;
	
	float4 viewport;
    float4 lightDirection;
	
    uint4 worldSize;
	
	uint sdfCount;
	uint particleCount;

    uint lightCount;
    float padding;
};

#ifdef __PSSL__
BUFFER(uint) voxelWorldData : register(t1);
BUFFER(uint) voxelModelData[] : register(t2) {};
#else
BUFFER(float4) voxelWorldData : register(t1);
BUFFER(float4) voxelModelData[] : register(t2);
#endif

struct PS_in
{
    float3 WorldPosition		: POSITION0;
    float4 NormScreenPosition	: POS_OUT;
	float4 Direction			: POSITION1;
	float3 AABBSize				: POSITION2;
	float Penetration			: POSITION3;
};

#include "SDFMarcher.hlsl"

MarchResult DepthMarch(float3 v3Origin, float3 v3Direction, float3 v3ChunkSize, int iMaxSteps) {
	MarchResult result;
	float3 v3InvDirection = 1.0 / v3Direction;
	
	int3 v3Position = floor(v3Origin);
	float3 v3SignedRayDirection = sign(v3Direction);
	float3 v3Distance = (v3Position - v3Origin + 0.5 + v3SignedRayDirection * 0.5) * v3InvDirection;
	float3 v3Mask = step(v3Distance.xyz, v3Distance.yxy) * step(v3Distance.xyz, v3Distance.zzx);
	
	for (int i = 0; i < iMaxSteps; i++) {	
		result.Color = GetVoxel(v3Position);
		
		if (result.Color.a > 0) {
			v3Distance = (v3Position - v3Origin + 0.5 - v3SignedRayDirection * 0.5) * v3InvDirection;
			result.Distance = max(v3Distance.x, max(v3Distance.y, v3Distance.z));	
			result.SmoothPosition = v3Origin + v3Direction * result.Distance;
			
			return result;
		}
		
		v3Mask = step(v3Distance.xyz, v3Distance.yxy) * step(v3Distance.xyz, v3Distance.zzx);
		v3Distance += v3Mask * v3SignedRayDirection * v3InvDirection;
		v3Position += v3Mask * v3SignedRayDirection;
		
		if (!IsInChunk(v3Position))
			break;
	}
	
	result.Color = float4(CLEAR_COLOR, 0.0);
	return result;
}

float GetDirectLightingDepth(float3 startPos)
{
    MarchResult tRes = March(
		startPos,
		-lightDirection.xyz,
		float3(
			float(worldSize.x),
			float(worldSize.y),
			float(worldSize.z)
		),
		128,
		false
	);

    return (tRes.Color.a == 0.0) ? 1.0 : 0.0;
}

struct PixelOut
{
    float ColorVoxel  : TAR_OUT0;
    float ColorShadow : TAR_OUT1;
};

PixelOut main(PS_in IN)
{
    PixelOut OUT;
    
    MarchResult depthRes = DepthMarch(IN.WorldPosition - camOffset.xyz, normalize(IN.Direction.xyz), IN.AABBSize, IN.Penetration);
    if (depthRes.Color.a == 0.0)
        discard;

    OUT.ColorVoxel = distance(depthRes.SmoothPosition, camPosition.xyz - camOffset.xyz);
    OUT.ColorShadow = GetDirectLightingDepth(depthRes.SmoothPosition - lightDirection.xyz);

    return OUT;
}