#pragma once

#include "Core/Platform/Rendering/RenderPassInc.h"
#include "Core/Platform/Rendering/RenderDefines.h"

class Shader;
class Buffer;

class VoxelPass : public PRenderPass
{
public:
	VoxelPass(
		PRenderContext* pContext, Shader* pVertex, Shader* pPixel, Sampler* pSampler,
		Mapper* pVoxelMapper, Mapper* pBrickMapper, Buffer* pCameraBuffer, Buffer* pAABBBuffer,
		View* pParticleTexture, View* pParticleDepthTexture, View* pPrepassTexture
	);

	virtual void Begin(PCommandEngine* pEngine) override;
};