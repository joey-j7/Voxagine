#pragma once

#include "Core/Platform/Rendering/Managers/TextureManager.h"

#include <vector>

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
	/* Texture IDs are descriptor array indices, so they have to be reused
	   rather than handed out monotonically: the array is a fixed size and the
	   shader indexes it with whatever ID a sprite carries. DXHeapManager did
	   exactly this with ReserveID/FreeID; without it, streaming worlds in and
	   out walks the IDs past the end of the array. */
	uint32_t AcquireID();
	void ReleaseID(uint32_t uiID);

	uint32_t m_uiNextID = 0;
	std::vector<uint32_t> m_FreeIDs;
};
