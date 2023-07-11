#define CLEAR_COLOR_COMPONENT 0.1
#define CLEAR_COLOR float3(CLEAR_COLOR_COMPONENT, CLEAR_COLOR_COMPONENT, CLEAR_COLOR_COMPONENT)
#define COLOR_CONVERTER 1 / 255.0

// glsl style mod
#define mod(x, y) (x - y * floor(x / y))

struct MarchResult
{
	float3 Position;
	float3 SmoothPosition;
	
	float Distance;
	float3 Normal;
	float2 UV;
	
	float3 Mask;
	float3 SRDirection;
	
	float4 Color;
};

static uint numStepsTaken = 0;

inline uint PosToVoxelID(uint3 v3Position)
{
	return v3Position.x + v3Position.y * worldSize.x + worldSize.x * worldSize.y * v3Position.z;
}

inline float4 GetVoxel(float3 v3Position) {
	uint ID = PosToVoxelID(v3Position);
	
#ifdef __PSSL__
    uint uiColor = voxelWorldData[ID];

	return float4(
        0xFF & (uiColor),
        0xFF & (uiColor >> 8),
        0xFF & (uiColor >> 16),
        0xFF & (uiColor >> 24)
    ) / 255.0;
#else
	return voxelWorldData[ID];
#endif
}

inline bool IsInChunk(int3 v3Position) {
	return (
		v3Position.x >= 0.0 && v3Position.y >= 0.0 && v3Position.z >= 0.0 &&
		v3Position.x < worldSize.x && v3Position.y < worldSize.y && v3Position.z < worldSize.z
	);
}

float3 v3LessThan(float3 v3First, float3 v3Second) {
	return float3(v3First.x < v3Second.x, v3First.y < v3Second.y, v3First.z < v3Second.z);
}

float Sum(float3 v3Vector) {
	return dot(v3Vector, float3(1.0, 1.0, 1.0));
}

float GetDistanceToWorld(float3 v3Position, float3 v3InvDirection, float3 v3ChunkSize) {
	float3 tMin = (float3(0.0, 0.0, 0.0) - v3Position) * v3InvDirection;
    float3 tMax = (v3ChunkSize - v3Position) * v3InvDirection;
    
	float3 t1 = min(tMin, tMax);
    float3 t2 = max(tMin, tMax);
	
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
	
    return ((tNear <= tFar) ? tNear : -1.0);
}

// Returns a vector that is orthogonal to u.
float3 GetOrthogonal(float3 u){
	u = normalize(u);
	float3 v = float3(0.99146, 0.11664, 0.05832); // Pick any normalized vector.
	return abs(dot(u, v)) > 0.99999 ? cross(u, float3(0, 1, 0)) : cross(u, v);
}

MarchResult March(float3 v3Origin, float3 v3Direction, float3 v3ChunkSize, int iMaxSteps, bool bDetailedResult) {
	float3 v3InvDirection = 1.0 / v3Direction;
	float fDistanceToWorld = GetDistanceToWorld(v3Origin, v3InvDirection, v3ChunkSize);
	bool bIsInChunk = IsInChunk(v3Origin);
	
	MarchResult result;
		
	if (bIsInChunk || fDistanceToWorld >= 0.0)
	{
		v3Origin += !bIsInChunk * (fDistanceToWorld * v3Direction);
		
		int3 v3Position = floor(v3Origin);
		float3 v3SignedRayDirection = sign(v3Direction);
		float3 v3Distance = (v3Position - v3Origin + 0.5 + v3SignedRayDirection * 0.5) * v3InvDirection;
		float3 v3Mask = step(v3Distance.xyz, v3Distance.yxy) * step(v3Distance.xyz, v3Distance.zzx);
		
		// Check if it's necessary to march
		// iMaxSteps *= WillRayEverHit(v3Position, v3InvDirection, v3ChunkSize) && v3Position.y >= 0.0;

		for (int i = 0; i < iMaxSteps; i++) {	
			result.Color = GetVoxel(v3Position);
			
			if (result.Color.a > 0) {
				v3Distance = (v3Position - v3Origin + 0.5 - v3SignedRayDirection * 0.5) * v3InvDirection;
				result.Distance = max(v3Distance.x, max(v3Distance.y, v3Distance.z));
				
				if (bDetailedResult) {
					result.Position = v3Position;
					result.SmoothPosition = v3Origin + v3Direction * result.Distance;
			
					result.Normal = -v3Mask * v3SignedRayDirection;
					
					float3 v3IntersectPlane = v3Position + v3LessThan(v3Direction, float3(0, 0, 0));
					float3 v3EndRayPos = v3Direction / Sum(v3Mask * v3Direction) * Sum(v3Mask * (v3IntersectPlane - v3Origin)) + v3Origin;
					
					result.UV = mod(
						float2(
							dot(v3Mask * v3EndRayPos.zxy, float3(1.0, 1.0, 1.0)),
							dot(v3Mask * v3EndRayPos.yzx, float3(1.0, 1.0, 1.0))
						),
						float2(1.0, 1.0)
					);
					
					if (abs(result.Normal.b) > 0.5)
						result.UV = float2(result.UV.y, result.UV.x);
					
					result.Mask = v3Mask;
					result.SRDirection = v3SignedRayDirection;
				}
				
				return result;
			}
			
			v3Mask = step(v3Distance.xyz, v3Distance.yxy) * step(v3Distance.xyz, v3Distance.zzx);
			v3Distance += v3Mask * v3SignedRayDirection * v3InvDirection;
			v3Position += v3Mask * v3SignedRayDirection;
			numStepsTaken++;
			
			if (!IsInChunk(v3Position))
			{
				break;
			}
		}
	}
	
	result.Color = float4(CLEAR_COLOR, 0.0);
	return result;
}