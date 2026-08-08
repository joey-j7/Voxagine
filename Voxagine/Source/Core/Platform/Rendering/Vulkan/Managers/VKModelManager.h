#pragma once

#include "Core/Platform/Rendering/Managers/ModelManager.h"

/* Owns the Mappers that hold model voxel data.
 *
 * The DX12 version also owned a DXHeapManager, because each model needed a
 * descriptor heap slot reserved up front. Vulkan has no equivalent: descriptor
 * sets come from the pool inside VKDescriptorLayout, allocated per pass rather
 * than per resource, so that responsibility disappears here. */
class VKModelManager : public ModelManager
{
public:
	VKModelManager(PRenderContext* pContext);
	virtual ~VKModelManager();

	virtual uint32_t AddModel(Mapper* pMapper) override;
	virtual void DestroyModel(uint32_t uiID) override;

private:
	uint32_t m_uiNextID = 0;
};
