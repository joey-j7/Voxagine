#pragma once

#include "Core/Platform/Rendering/Managers/ModelManager.h"
#include "Core/Platform/Rendering/RenderDefines.h"

class DXHeapManager;

class DXModelManager : public ModelManager
{
public:
	DXModelManager(PRenderContext* pContext, DXHeapManager* pManager = nullptr);
	virtual ~DXModelManager();

	virtual uint32_t AddModel(Mapper* pMapper) override;
	virtual void DestroyModel(uint32_t uiID) override;

	DXHeapManager* GetHeapManager() const { return m_pHeapManager; }

private:
	bool m_bIsSRVOwner = false;
	DXHeapManager* m_pHeapManager = nullptr;
};