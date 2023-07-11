#pragma once

#include "Core/Platform/Rendering/Managers/IDManager.h"

#include "Core/Platform/Rendering/Objects/Mapper.h"
#include "Core/Platform/Rendering/RenderDefines.h"

#include <unordered_map>
#include <memory>

class RenderContext;

class ModelManager
{
public:
	ModelManager(PRenderContext* pContext);
	virtual ~ModelManager() = default;

	virtual uint32_t AddModel(Mapper* pMapper) = 0;
	virtual void DestroyModel(uint32_t uiID) = 0;

protected:
	PRenderContext* m_pContext = nullptr;

	std::unordered_map<uint32_t, std::unique_ptr<Mapper>> m_pMappers;
};