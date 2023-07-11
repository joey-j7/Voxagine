#pragma once

#include "Core/ECS/Systems/Rendering/Buffers/Structures/StructuredVoxelBuffer.h"
#include "Core/Platform/Rendering/RenderPassInc.h"
#include "Core/Platform/Rendering/RenderDefines.h"

class Shader;
class Buffer;

class DebugPass : public PRenderPass
{
public:
	DebugPass(
		PRenderContext* pContext, Shader* pVertex, Shader* pPixel,
		Buffer* pCameraBuffer, Buffer* pAABBBuffer
	);

	virtual void Begin(PCommandEngine* pEngine) override;
};