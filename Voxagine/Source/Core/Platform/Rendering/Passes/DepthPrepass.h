#pragma once

#include "Core/Platform/Rendering/RenderPassInc.h"
#include "Core/Platform/Rendering/RenderDefines.h"

class Shader;
class Buffer;

/* Low-resolution depth prepass (RENDERING_PLAN.md phase 3).
 *
 * One camera ray per texel, walked at brick granularity only, writing the
 * distance from the camera at which the ray first enters an occupied brick -
 * or k_fMiss when it never does. VoxelPass takes the minimum over a 3x3
 * neighbourhood of that and starts its own march there, so the empty prefix of
 * the ray is walked once per texel instead of once per pixel.
 *
 * Full screen rather than the AABB proxy cubes VoxelPass rasterizes: a proxy
 * fragment's march starts at that box's face, so which instance won the depth
 * test would change the recorded distance, and a prepass texel and the pixels
 * under it do not have to agree on that. A camera ray has no such ambiguity. */
class DepthPrepass : public PRenderPass
{
public:
	/* Fraction of the render resolution the prepass target is sized at. Must
	   match PREPASS_SCALE in Game/Engine/Assets/Shaders/Defines.hlsl - the
	   consumer converts its own pixel coordinate to a prepass texel with it,
	   and the prepass reconstructs its ray direction with it. */
	static const float k_fScale;

	/* Written for a ray that enters no occupied brick, and cleared to. Any
	   value at or above k_fMissTest means "no information"; the consumer then
	   skips nothing and marches from its own origin. */
	static const float k_fMiss;

	DepthPrepass(
		PRenderContext* pContext, Shader* pVertex, Shader* pPixel,
		Buffer* pCameraBuffer, Mapper* pVoxelMapper, Mapper* pBrickMapper
	);

	/* Draws nothing while RenderContext::IsDepthPrepassEnabled is false; the
	   target then holds the k_fMiss it was cleared to, which the marcher reads
	   as "no information" and skips nothing. That is the A/B, and it costs the
	   consumer nothing to express - there is no second voxel shader. */
	virtual void Begin(PCommandEngine* pEngine) override;
};
