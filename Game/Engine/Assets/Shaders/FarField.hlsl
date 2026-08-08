/* Far-field LOD volume - RENDERING_PLAN.md phase 4. Include after declaring:

     VOXEL_RW_BUFFER farFieldData;
     RW_STRUCTURED_BUFFER(uint) farFieldBrickData;

   The resident voxel window is a sliding 3x3 chunk view of a level that is up
   to 6x6, so past its edge there is simply nothing on the GPU to hit - which is
   why the horizon has always been the window's boundary. This is the rest of
   the level at FARFIELD_SIZE voxels per cell, built once at load by
   FarFieldBaker and never updated.

   Three coordinate spaces, and confusing them is the main way to get this
   wrong:

     - *window* space - voxels, origin at the resident window's corner. What
                        SDFMarcher.hlsl marches in, and what
                        VoxelRenderer.ps.hlsl calls world space.
     - *level* space  - voxels, origin at the level's corner.
                        level = window + camOffset.
     - *cell* space   - this volume's cells: level * FARFIELD_INV_SIZE.

   The marcher below works entirely in cell space, because a cell there is unit
   sized and the same DDA as SDFMarcher.hlsl's applies unchanged; distances come
   back out in world units. FarFieldVolume.h holds the same three-space note on
   the C++ side. */

struct FarFieldResult
{
	float4 Color;

	/* World units from the origin passed in. */
	float Distance;

	float3 Normal;
};

inline uint3 GetFarFieldBrickGridSize() {
	return (farFieldSize.xyz + (BRICK_SIZE - 1)) >> BRICK_SHIFT;
}

inline uint PosToFarFieldID(uint3 v3Cell) {
	return v3Cell.x + v3Cell.y * farFieldSize.x + farFieldSize.x * farFieldSize.y * v3Cell.z;
}

inline bool IsInFarField(int3 v3Cell) {
	return (
		v3Cell.x >= 0 && v3Cell.y >= 0 && v3Cell.z >= 0 &&
		v3Cell.x < int(farFieldSize.x) && v3Cell.y < int(farFieldSize.y) && v3Cell.z < int(farFieldSize.z)
	);
}

inline bool IsFarFieldBrickInWorld(int3 v3Brick) {
	int3 v3Grid = int3(GetFarFieldBrickGridSize());

	return (
		v3Brick.x >= 0 && v3Brick.y >= 0 && v3Brick.z >= 0 &&
		v3Brick.x < v3Grid.x && v3Brick.y < v3Grid.y && v3Brick.z < v3Grid.z
	);
}

inline bool IsFarFieldBrickOccupied(int3 v3Brick) {
	uint3 v3Grid = GetFarFieldBrickGridSize();
	uint uiID = uint(v3Brick.x) + uint(v3Brick.y) * v3Grid.x + uint(v3Brick.z) * v3Grid.x * v3Grid.y;

	return farFieldBrickData[uiID] != 0;
}

/* The same two-level walk MarchBricks does, over the cell grid instead of the
   voxel grid, with the fine level's per-hit UV and mask work dropped - nothing
   shades a far-field cell finely enough to want them. Kept as its own copy
   rather than a parameterization of MarchBricks because HLSL has no way to
   swap the two buffers a function reads without a macro that would have to be
   expanded twice anyway, and the two walks are free to diverge: this one has no
   shadow ray to stay consistent with.

   v3Origin is in *cell* space. Distance comes back in world units. */
FarFieldResult MarchFarField(float3 v3Origin, float3 v3Direction, int iMaxBrickSteps) {
	float3 v3InvDirection = 1.0 / v3Direction;
	float3 v3SignedRayDirection = sign(v3Direction);

	FarFieldResult result;
	result.Color = float4(0.0, 0.0, 0.0, 0.0);
	result.Distance = 0.0;
	result.Normal = float3(0.0, 1.0, 0.0);

	int3 v3Cell = int3(floor(v3Origin));

	float3 v3BrickOrigin = v3Origin * BRICK_INV_SIZE;
	int3 v3Brick = int3(floor(v3BrickOrigin));

	float3 v3BrickDistance = (float3(v3Brick) - v3BrickOrigin + 0.5 + v3SignedRayDirection * 0.5) * v3InvDirection;
	float3 v3BrickMask = float3(0.0, 0.0, 0.0);

	/* Cell-space parameter at which the ray entered v3Brick; unused for the
	   brick it starts inside. */
	float fBrickEntry = 0.0;
	bool bFirstBrick = true;

	for (int b = 0; b < iMaxBrickSteps; b++) {
		if (!IsFarFieldBrickInWorld(v3Brick))
			break;

		if (IsFarFieldBrickOccupied(v3Brick)) {
			int3 v3Position;
			float3 v3Mask;

			if (bFirstBrick) {
				v3Position = v3Cell;
			}
			else {
				/* Reconstructed from the crossing parameter, then clamped into
				   the brick: the entry point lands exactly on a brick face, so
				   floor() of it can name the cell on the far side when a
				   direction component is negative, and float error can push it
				   either way. The cell it would otherwise pick belongs to a
				   brick this walk already cleared as empty. */
				float3 v3Entry = v3Origin + v3Direction * fBrickEntry;

				v3Position = clamp(
					int3(floor(v3Entry)),
					v3Brick << BRICK_SHIFT,
					(v3Brick << BRICK_SHIFT) + (BRICK_SIZE - 1)
				);
			}

			float3 v3Distance = (float3(v3Position) - v3Origin + 0.5 + v3SignedRayDirection * 0.5) * v3InvDirection;

			v3Mask = bFirstBrick
				? step(v3Distance.xyz, v3Distance.yxy) * step(v3Distance.xyz, v3Distance.zzx)
				: v3BrickMask;

			for (int v = 0; v < FARFIELD_MAX_CELL_STEPS; v++) {
				/* An edge brick can cover cells past farFieldSize when the grid
				   is not a whole number of bricks. The volume is a box, so a ray
				   that leaves it never re-enters. */
				if (!IsInFarField(v3Position))
					return result;

				result.Color = farFieldData[PosToFarFieldID(uint3(v3Position))];

				if (result.Color.a > 0) {
					v3Distance = (float3(v3Position) - v3Origin + 0.5 - v3SignedRayDirection * 0.5) * v3InvDirection;

					result.Distance = max(v3Distance.x, max(v3Distance.y, v3Distance.z)) * FARFIELD_SIZE_F;
					result.Normal = -v3Mask * v3SignedRayDirection;

					return result;
				}

				v3Mask = step(v3Distance.xyz, v3Distance.yxy) * step(v3Distance.xyz, v3Distance.zzx);
				v3Distance += v3Mask * v3SignedRayDirection * v3InvDirection;
				v3Position += int3(v3Mask * v3SignedRayDirection);

				if (any((v3Position >> BRICK_SHIFT) != v3Brick))
					break;
			}
		}

		v3BrickMask = step(v3BrickDistance.xyz, v3BrickDistance.yxy) * step(v3BrickDistance.xyz, v3BrickDistance.zzx);
		fBrickEntry = min(v3BrickDistance.x, min(v3BrickDistance.y, v3BrickDistance.z)) * BRICK_SIZE_F;

		v3BrickDistance += v3BrickMask * v3SignedRayDirection * v3InvDirection;
		v3Brick += int3(v3BrickMask * v3SignedRayDirection);

		bFirstBrick = false;
	}

	result.Color = float4(0.0, 0.0, 0.0, 0.0);
	return result;
}

/* Near and far crossings of the axis-aligned box [v3Min, v3Max], as
   float2(tNear, tFar). tNear > tFar means the ray misses it entirely.

   SDFMarcher.hlsl's GetDistanceToWorld answers half of this for a box at the
   origin; both boxes here are somewhere else in level space, and both crossings
   are needed - the window's far one to know where to start, the volume's near
   one to know it is being entered rather than missed. */
float2 GetBoxCrossings(float3 v3Origin, float3 v3InvDirection, float3 v3Min, float3 v3Max) {
	float3 tMin = (v3Min - v3Origin) * v3InvDirection;
	float3 tMax = (v3Max - v3Origin) * v3InvDirection;

	float3 t1 = min(tMin, tMax);
	float3 t2 = max(tMin, tMax);

	return float2(
		max(max(t1.x, t1.y), t1.z),
		min(min(t2.x, t2.y), t2.z)
	);
}

/* Marches the level for a pixel the detail window left empty.
 *
 * v3LevelOrigin is the camera in level space and v3Direction its normalized
 * ray. The march deliberately starts *past* the detail window: the far field
 * covers the whole level, window included, and at four voxels per cell its copy
 * of the near geometry is conservatively fatter than the real thing - so
 * marching it from the camera would paint blobs over exactly the empty space
 * the window just proved empty. Skipping to the window's far face means the far
 * field is only ever consulted where the window has no answer, and puts the
 * seam between the two at the window's boundary, which is where phase 6.1's fog
 * will hide it.
 *
 * Returns alpha 0 when there is no far field, or nothing along the ray. */
FarFieldResult MarchFarFieldFromWindow(float3 v3LevelOrigin, float3 v3Direction) {
	FarFieldResult miss;
	miss.Color = float4(0.0, 0.0, 0.0, 0.0);
	miss.Distance = 0.0;
	miss.Normal = float3(0.0, 1.0, 0.0);

	/* No far field: a level the window already covers, a build that found
	   nothing, or the runtime toggle off. */
	if (farFieldSize.x == 0 || farFieldSize.y == 0 || farFieldSize.z == 0)
		return miss;

	float3 v3InvDirection = 1.0 / v3Direction;

	/* The volume itself, in level space. Cells times FARFIELD_SIZE rather than
	   the level size the C++ side was given: those differ when a level is not a
	   whole number of cells, and it is the cells that exist. */
	float3 v3VolumeMin = float3(0.0, 0.0, 0.0);
	float3 v3VolumeMax = float3(farFieldSize.xyz) * FARFIELD_SIZE_F;

	float2 v2Volume = GetBoxCrossings(v3LevelOrigin, v3InvDirection, v3VolumeMin, v3VolumeMax);

	/* Behind the ray, or missed. */
	if (v2Volume.x > v2Volume.y || v2Volume.y < 0.0)
		return miss;

	/* The detail window, in level space. */
	float3 v3WindowMin = sceneCamOffset.xyz;
	float3 v3WindowMax = sceneCamOffset.xyz + float3(worldSize.xyz);

	float2 v2Window = GetBoxCrossings(v3LevelOrigin, v3InvDirection, v3WindowMin, v3WindowMax);

	/* Where to begin: past the detail window if the ray crosses it, and not
	   before the volume itself begins.

	   The second half of that is not a formality. A camera above the level
	   looking at a distant part of it produces a ray that never touches the
	   window at all *and* starts outside the volume - so a march that began at
	   the camera would find its first brick out of bounds and give up on the
	   spot, which is exactly the "see the far end of the level" case this phase
	   exists for. */
	float fWindowExit = (v2Window.x <= v2Window.y) ? max(v2Window.y, 0.0) : 0.0;

	float fStart = max(fWindowExit, max(v2Volume.x, 0.0)) + FARFIELD_ENTRY_EPSILON;

	if (fStart > v2Volume.y)
		return miss;

	/* Nudged inside, so that floor() on a point sitting exactly on a face
	   cannot name the cell on the far side of it. */
	float3 v3Start = clamp(
		v3LevelOrigin + v3Direction * fStart,
		v3VolumeMin + WORLD_ENTRY_EPSILON,
		v3VolumeMax - WORLD_ENTRY_EPSILON
	);

	/* Budget the walk by the volume's diagonal in bricks, so nothing at the far
	   end of the level is truncated. */
	int iMaxBrickSteps = int(length(float3(farFieldSize.xyz)) * BRICK_INV_SIZE) + 2;

	FarFieldResult result = MarchFarField(v3Start * FARFIELD_INV_SIZE, v3Direction, iMaxBrickSteps);

	/* Distances stay relative to the origin the caller passed, so they can be
	   compared against the analytic ground plane. */
	if (result.Color.a > 0.0)
		result.Distance += fStart;

	return result;
}

/* Ambient plus a single N.L term against the scene light. No shadow ray and no
   AO - see FARFIELD_AMBIENT. */
float4 ShadeFarField(FarFieldResult far) {
	float fDifference = clamp(dot(far.Normal, -lightDirection.xyz), 0.0, 1.0);
	float fLight = fDifference * (1.0 - FARFIELD_AMBIENT) + FARFIELD_AMBIENT;

	return float4(far.Color.xyz * fLight, 1.0);
}

/* The whole background: far-field geometry, then the endless ground plane, then
   sky. Include after also declaring

     inline float4 PostFxGetVoxel(float3 v3Position);

   which is where the ground's colour comes from. Shared by
   PostProcessing.ps.hlsl and PostProcessing.Debug.ps.hlsl, which were already
   carrying identical copies of the sky-and-ground half of this.

   Why this lives in post processing rather than in the voxel pass: the voxel
   pass only rasterizes AABB proxy cubes, and by definition no proxy covers the
   part of the level that is not resident. A pixel showing only far-field
   geometry is never rasterized there at all. Post processing is the one pass
   that runs for every pixel.

   Rays come from invMvp rather than Camera.hlsl's GetRay, which is what the sky
   and ground used to use. GetRay rebuilds the ray from FOV and aspect via mv,
   and mv is only the inverse view matrix for a yaw-only camera - the sky could
   not tell, since it reads only rayDirection.y, but far-field geometry can: a
   ray that drifts from the one the voxel pass projected with puts the far field
   visibly out of register with the window at the seam. Under an orthographic
   projection this reconstruction is as meaningless as GetRay was, for the same
   reason SDFPrepass gives up there. */
float4 GetBackground(float2 v2PixelPosition)
{
	float2 v2NDC = float2(
		2.0 * v2PixelPosition.x / viewport.x - 1.0,
		1.0 - 2.0 * v2PixelPosition.y / viewport.y
	);

	/* The scene camera, not the live one: this pass composites an image the
	   voxel pass rendered a submission ago, and a ray built from a camera that
	   has since moved slides against it. See CameraData.hlsl. */
	float4 v4World = mul(sceneInvMvp, float4(v2NDC, 0.5, 1.0));

	/* Level space: sceneCamPosition is absolute, and the window offset is what
	   converts it to the space SDFMarcher.hlsl works in. */
	float3 v3LevelOrigin = sceneCamPosition.xyz;
	float3 v3Direction = v4World.xyz / v4World.w - v3LevelOrigin;

	float fDirectionLength = length(v3Direction);

	if (!(fDirectionLength > 0.0) || v4World.w == 0.0)
		return SKY_COLOR;

	v3Direction /= fDirectionLength;

	FarFieldResult far = MarchFarFieldFromWindow(v3LevelOrigin, v3Direction);

	/* Endless tiled ground, in window space - the same plane in both, since the
	   window offset has no Y component.

	   GROUND_PLANE_HEIGHT, not zero. The chunk ground plane is a layer of voxels
	   at integer y = 0, and a voxel at integer y spans [y, y+1], so its top face
	   - the surface the marcher actually reports a hit on - is at y = 1. Solving
	   for y = 0 puts this plane one unit below the window's own ground, and the
	   lip between them is visible edge-on at the window's boundary as a row of
	   teeth along the horizon. */
	float3 v3WindowOrigin = v3LevelOrigin - sceneCamOffset.xyz;

	if (v3WindowOrigin.y >= GROUND_PLANE_HEIGHT && v3Direction.y < 0.0)
	{
		float fGround = (GROUND_PLANE_HEIGHT - v3WindowOrigin.y) / v3Direction.y;

		/* Whichever is nearer. The far field stops at the level's edge and the
		   ground does not, so past the level this is always the ground - which
		   is what keeps far-field geometry reading as standing on it rather
		   than floating over a horizon. */
		if (far.Color.a == 0.0 || fGround < far.Distance)
		{
			float3 v3End = v3WindowOrigin + v3Direction * fGround;

			float3 v3GroundPos;
			v3GroundPos.x = abs(fmod(v3End.x, float(worldSize.x)));
			v3GroundPos.y = 0.0;
			v3GroundPos.z = abs(fmod(v3End.z, float(worldSize.z)));

			/* Lit like any other up-facing surface the shadow ray found nothing
			   on, rather than through a constant. This used to be a flat 0.6,
			   which is not what the voxel pass gives the very same ground layer
			   a few metres nearer the camera - the level's floor visibly
			   darkened at the window's edge. It still cannot match exactly,
			   since the window's copy also gets AO and the shine line, but the
			   ambient and N.L terms are the whole of the difference now. */
			FarFieldResult ground;
			ground.Color = PostFxGetVoxel(v3GroundPos);
			ground.Distance = fGround;
			ground.Normal = float3(0.0, 1.0, 0.0);

			return ShadeFarField(ground);
		}
	}

	if (far.Color.a > 0.0)
		return ShadeFarField(far);

	return SKY_COLOR;
}
