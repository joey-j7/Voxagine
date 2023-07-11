#pragma once

#include "Core/Platform/Rendering/RenderPassInc.h"
#include "Core/Platform/Rendering/RenderDefines.h"

class Shader;
class Buffer;

class UIPass : public PRenderPass
{
public:
	UIPass(
		PRenderContext* pContext, Shader* pVertex, Shader* pPixel, Sampler* pSamplerPoint,
		Buffer* pCameraBuffer, Buffer* pSpriteBuffer
	);

	virtual void Begin(PCommandEngine* pEngine) override;
};