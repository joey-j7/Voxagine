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

/* --- Occupancy bricks (RENDERING_PLAN.md phase 2) -------------------------
   voxelBrickData holds one count of occupied voxels per BRICK_SIZE^3 block of
   the resident window, maintained on the CPU by VoxelBrickGrid. Only "is it
   zero" is read here; the count exists so that destroying one voxel can
   decrement its brick without rescanning the other 511.

   Linearization is the voxel convention (rule 4) scaled down:
   x + y*B.x + z*B.x*B.y with B = ceil(worldSize / BRICK_SIZE). The C++ side
   computes B the same way - VoxelBrickGrid::Resize. */

inline uint3 GetBrickGridSize() {
	return (worldSize.xyz + (BRICK_SIZE - 1)) >> BRICK_SHIFT;
}

inline uint PosToBrickID(int3 v3Brick) {
	uint3 v3Grid = GetBrickGridSize();
	return uint(v3Brick.x) + uint(v3Brick.y) * v3Grid.x + uint(v3Brick.z) * v3Grid.x * v3Grid.y;
}

inline bool IsBrickInWorld(int3 v3Brick) {
	int3 v3Grid = int3(GetBrickGridSize());

	return (
		v3Brick.x >= 0 && v3Brick.y >= 0 && v3Brick.z >= 0 &&
		v3Brick.x < v3Grid.x && v3Brick.y < v3Grid.y && v3Brick.z < v3Grid.z
	);
}

inline bool IsBrickOccupied(int3 v3Brick) {
	return voxelBrickData[PosToBrickID(v3Brick)] != 0;
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

/* Two-level DDA over the resident window: an outer walk that steps whole
   BRICK_SIZE^3 bricks and skips empty ones for free, and an inner voxel walk
   that only ever runs inside a brick the CPU says holds something.

   Both levels share the ray's *original* origin, so every hit-time value below
   - Distance, SmoothPosition, UV - stays origin-relative no matter which brick
   the fine walk restarted in. That is what makes restarting cheap: the fine
   DDA state is a closed-form function of (origin, direction, voxel), not an
   accumulation, so it can be re-derived at any point along the ray.

   Replaces the flat single-level walk that the 700-step cap and the shadow
   ray's 2x stride existed to bound - see RENDERING_PLAN.md phase 2. */
MarchResult MarchBricks(float3 v3Origin, float3 v3Direction, int iMaxBrickSteps) {
	float3 v3InvDirection = 1.0 / v3Direction;
	float3 v3SignedRayDirection = sign(v3Direction);

	MarchResult result;
	result.Color = float4(CLEAR_COLOR, 0.0);

	/* Brick space is world space scaled by 1/BRICK_SIZE, so its cells are unit
	   sized and the same DDA applies; a parameter t in brick space is
	   BRICK_SIZE * t in world space. */
	float3 v3BrickOrigin = v3Origin * BRICK_INV_SIZE;
	int3 v3Brick = int3(floor(v3BrickOrigin));

	float3 v3BrickDistance = (float3(v3Brick) - v3BrickOrigin + 0.5 + v3SignedRayDirection * 0.5) * v3InvDirection;
	float3 v3BrickMask = float3(0.0, 0.0, 0.0);

	/* World-space parameter at which the ray entered v3Brick. Unused for the
	   brick the ray starts inside. */
	float fBrickEntry = 0.0;
	bool bFirstBrick = true;

	for (int b = 0; b < iMaxBrickSteps; b++) {
		if (!IsBrickInWorld(v3Brick))
			break;

		if (IsBrickOccupied(v3Brick)) {
			int3 v3Position;
			float3 v3Mask;

			if (bFirstBrick) {
				v3Position = int3(floor(v3Origin));

				/* A march that begins *inside* an occupied voxel has no entry
				   face, so there is no normal to report. The DDA below would
				   derive one anyway, from the next crossing rather than a
				   crossing already made - and for an origin sitting on a voxel
				   face, the nearest crossing is that same face. The result is a
				   horizontal normal on a piece of floor.

				   That is what draws the strip along the resident window's
				   boundary. VoxelRenderer.vs.hlsl clamps the proxy cube's
				   surface to worldSize, so the edge fragments start the march
				   exactly on the window's face and immediately inside the
				   ground layer. Every pixel there came out +/-X or +/-Z where
				   its neighbours were +Y: dark, because it faces away from the
				   light, with GetShineLine's vertical-wall branch adding a lit
				   rim along the top.

				   This is the place for the test rather than MarchDiffuse's
				   out-of-window path, which only sees the fragments whose
				   clamped origin floors to *outside* the window. The ones that
				   land a fraction inside come straight here, and fixing only
				   the first turned the strip into a dashed version of itself.

				   Reporting a miss hands the pixel to the background, which is
				   the honest answer at a boundary that is a cut through the
				   world rather than a surface in it: the endless ground plane
				   and the far-field volume both already draw what is on the
				   other side, so the world continues instead of ending in a
				   wall. */
				if (IsInChunk(v3Position) && GetVoxel(float3(v3Position)).a > 0.0) {
					result.Color = float4(CLEAR_COLOR, 0.0);
					return result;
				}
			}
			else {
				/* Reconstruct the entry point from the crossing parameter. It
				   lands exactly on a brick face, so floor() of it can name the
				   voxel on the far side for a negative direction component -
				   and float error can push it either way. Clamping into the
				   brick fixes both: the voxel it would otherwise pick belongs
				   to a brick this walk has already cleared as empty. */
				float3 v3Entry = v3Origin + v3Direction * fBrickEntry;

				v3Position = clamp(
					int3(floor(v3Entry)),
					v3Brick << BRICK_SHIFT,
					(v3Brick << BRICK_SHIFT) + (BRICK_SIZE - 1)
				);
			}

			float3 v3Distance = (float3(v3Position) - v3Origin + 0.5 + v3SignedRayDirection * 0.5) * v3InvDirection;

			/* The face the ray crossed to enter the brick is the same face it
			   crosses to enter that brick's first voxel, so the outer mask is
			   the correct normal for a hit on the very first sample. */
			v3Mask = bFirstBrick
				? step(v3Distance.xyz, v3Distance.yxy) * step(v3Distance.xyz, v3Distance.zzx)
				: v3BrickMask;

			for (int v = 0; v < BRICK_MAX_VOXEL_STEPS; v++) {
				/* An edge brick can cover voxels past worldSize when the window
				   is not a whole number of bricks. The window is a box, so a ray
				   that leaves it never re-enters - this ends the whole march,
				   not just the inner walk. */
				if (!IsInChunk(v3Position)) {
					result.Color = float4(CLEAR_COLOR, 0.0);
					return result;
				}

				result.Color = GetVoxel(v3Position);

				if (result.Color.a > 0) {
					v3Distance = (float3(v3Position) - v3Origin + 0.5 - v3SignedRayDirection * 0.5) * v3InvDirection;
					result.Distance = max(v3Distance.x, max(v3Distance.y, v3Distance.z));
					result.SmoothPosition = v3Origin + v3Direction * result.Distance;

					result.Position = v3Position;
					result.Normal = -v3Mask * v3SignedRayDirection;

					float3 v3IntersectPlane = float3(v3Position) + v3LessThan(v3Direction, float3(0, 0, 0));
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

					return result;
				}

				v3Mask = step(v3Distance.xyz, v3Distance.yxy) * step(v3Distance.xyz, v3Distance.zzx);
				v3Distance += v3Mask * v3SignedRayDirection * v3InvDirection;
				v3Position += int3(v3Mask * v3SignedRayDirection);

				numStepsTaken++;

				if (any((v3Position >> BRICK_SHIFT) != v3Brick))
					break;
			}
		}

		/* Step to the next brick. The crossing parameter has to be read before
		   the step, since that is where the next brick is entered. */
		v3BrickMask = step(v3BrickDistance.xyz, v3BrickDistance.yxy) * step(v3BrickDistance.xyz, v3BrickDistance.zzx);
		fBrickEntry = min(v3BrickDistance.x, min(v3BrickDistance.y, v3BrickDistance.z)) * BRICK_SIZE_F;

		v3BrickDistance += v3BrickMask * v3SignedRayDirection * v3InvDirection;
		v3Brick += int3(v3BrickMask * v3SignedRayDirection);

		bFirstBrick = false;
	}

	result.Color = float4(CLEAR_COLOR, 0.0);
	return result;
}

MarchResult MarchLight(float3 v3Origin, float3 v3Direction, int iMaxBrickSteps) {
	return MarchBricks(v3Origin, v3Direction, iMaxBrickSteps);
}

MarchResult MarchDiffuse(float3 v3Origin, float3 v3Direction, int iMaxBrickSteps) {
	/* Callers may pass an origin outside worldSize bounds - the AABB proxy
	   cubes are clamped to the window, but a camera sitting on the boundary
	   still produces one. MarchBricks' own bounds tests cover the walk; this
	   covers the origin voxel it starts from.

	   Entering the window and marching from there - rather than reporting a
	   miss, as this did - is what keeps a grazing ray from punching holes
	   along the window's silhouette. */
	if (!IsInChunk(int3(floor(v3Origin)))) {
		MarchResult miss;
		miss.Color = float4(CLEAR_COLOR, 0.0);

		float fToWorld = GetDistanceToWorld(v3Origin, 1.0 / v3Direction, float3(worldSize.xyz));

		/* Negative means the window is behind the ray, or it misses entirely. */
		if (fToWorld < 0.0)
			return miss;

		float3 v3Entry = clamp(
			v3Origin + v3Direction * fToWorld,
			float3(WORLD_ENTRY_EPSILON, WORLD_ENTRY_EPSILON, WORLD_ENTRY_EPSILON),
			float3(worldSize.xyz) - WORLD_ENTRY_EPSILON
		);

		if (!IsInChunk(int3(floor(v3Entry))))
			return miss;

		MarchResult result = MarchBricks(v3Entry, v3Direction, iMaxBrickSteps);

		/* Distance stays relative to the origin the caller passed;
		   SmoothPosition is absolute already. */
		if (result.Color.a > 0.0)
			result.Distance += fToWorld;

		return result;
	}

	return MarchBricks(v3Origin, v3Direction, iMaxBrickSteps);
}

/* Single-level walk, kept as-is. Its only caller is SDFDepth.ps.hlsl, which
   compiles but is loaded by nothing (see RENDERING_PLAN.md, "Ground truth") -
   there was no behaviour to preserve by porting it onto the brick walk, and
   no way to verify the port either. */
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
		
		for (int i = 0; i < iMaxSteps; i++) {	
			result.Color = GetVoxel(v3Position);
			
			if (result.Color.a > 0)
			{
				v3Distance = (v3Position - v3Origin + 0.5 - v3SignedRayDirection * 0.5) * v3InvDirection;
				result.Distance = max(v3Distance.x, max(v3Distance.y, v3Distance.z));
	
				result.SmoothPosition = v3Origin + v3Direction * result.Distance;

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
				break;
		}
	}
	
	result.Color = float4(CLEAR_COLOR, 0.0);
	return result;
}