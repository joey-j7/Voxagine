#include "pch.h"
#include "VKModelManager.h"

#include "Core/Platform/Rendering/Vulkan/VKRenderContext.h"

VKModelManager::VKModelManager(PRenderContext* pContext) : ModelManager(pContext)
{
}

VKModelManager::~VKModelManager() = default;

uint32_t VKModelManager::AddModel(Mapper* pMapper)
{
	if (pMapper == nullptr)
		return UINT32_MAX;

	const uint32_t uiID = m_uiNextID++;
	m_pMappers[uiID] = std::unique_ptr<Mapper>(pMapper);

	return uiID;
}

void VKModelManager::DestroyModel(uint32_t uiID)
{
	m_pMappers.erase(uiID);
}
