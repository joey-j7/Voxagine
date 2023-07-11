#pragma once

#include "Core/Platform/Rendering/RenderContext.h"
#include "Core/Platform/Rendering/RenderPassInc.h"

class ImguiSystem;

class GLRenderContext : public RenderContext {
public:
	GLRenderContext(Platform* pPlatform) : RenderContext(pPlatform) {};
	virtual ~GLRenderContext() = default;

	virtual void Initialize() override {};
	virtual void Deinitialize() override {};

	virtual void Clear() override {};
	virtual bool Present() override { return false; };

	virtual bool OnResize(uint32_t uiWidth, uint32_t uiHeight) override { return false; };

private:
	virtual void LoadTexture(TextureReference* pTextureReference) override {};
	virtual TextureReadData* ReadTexture(const std::string& texturePath) override { return nullptr; };
	virtual void DestroyTexture(const TextureReference* pTextureRef) override {};

	virtual void LoadShader(ShaderReference* pTextureReference) override {};
	virtual void DestroyShader(const ShaderReference* pTextureRef) override {};

	void PresentVoxels(RenderData& renderData) {};
};