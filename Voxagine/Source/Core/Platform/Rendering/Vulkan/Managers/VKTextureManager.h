#pragma once

#include "Core/Platform/Rendering/Managers/TextureManager.h"

class VKRenderContext;
class CommandEngine;

/* Creates and owns the Views backing texture resources.
 *
 * As with VKModelManager, the DXHeapManager slot reservation has no Vulkan
 * counterpart; descriptor sets are allocated per pass from VKDescriptorLayout. */
class VKTextureManager : public TextureManager
{
public:
	VKTextureManager(VKRenderContext* pContext);
	virtual ~VKTextureManager();

	virtual uint32_t CreateTexture(CommandEngine* pEngine, const std::string& sName,
	                               uint8_t* pData, UVector2 uSize) override;

	virtual uint32_t CreateEmptyTexture() override;

	virtual bool LoadTexture(CommandEngine* pEngine, TextureReference* pTextureReference) override;
	virtual void DestroyTexture(const TextureReference* pTextureReference) override;

private:
	uint32_t m_uiNextID = 0;
};
