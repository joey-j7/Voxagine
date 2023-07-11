#pragma once

#include "Core/ECS/Systems/Rendering/Buffers/Structures/StructuredVoxelBuffer.h"
#include "Core/Platform/Rendering/RenderPassInc.h"
#include "Core/Platform/Rendering/RenderDefines.h"

class Shader;
class Buffer;
class Sampler;

class PostProcessingPass : public PRenderPass
{
public:
	PostProcessingPass(
		PRenderContext* pContext, Shader* pVertex, Shader* pPixel, Sampler* pSampler,
		Buffer* pCameraBuffer, std::vector<View*> pTextures
	);
};