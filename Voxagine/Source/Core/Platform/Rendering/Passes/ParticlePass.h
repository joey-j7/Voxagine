#pragma once

#include "Core/ECS/Systems/Rendering/Buffers/Structures/StructuredVoxelBuffer.h"
#include "Core/Platform/Rendering/RenderPassInc.h"
#include "Core/Platform/Rendering/RenderDefines.h"

class Shader;
class Buffer;

class ParticlePass : public PRenderPass
{
	friend class RenderContext;
	friend class PhysicsSystem;

public:
	ParticlePass(
		PRenderContext* pContext, Shader* pVertex, Shader* pPixel, Buffer* pCameraBuffer, Mapper* pMapper, Sampler* pSamplerPoint
	);

	virtual void Begin(PCommandEngine* pEngine) override;

protected:
	Mapper*& GetMapper() { return m_Data.m_Mappers.front(); }
	uint32_t m_uiParticleCount = 0;
};