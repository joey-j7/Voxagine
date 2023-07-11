#include "pch.h"
#include "DXModelManager.h"

#include "Core/Platform/Rendering/RenderContextInc.h"
#include "Core/Platform/Rendering/DX12/Managers/DXHeapManager.h"

DXModelManager::DXModelManager(PRenderContext* pContext, DXHeapManager* pManager) : ModelManager(pContext)
{
	if (pManager)
	{
		m_pHeapManager = pManager;
		return;
	}

	m_bIsSRVOwner = true;
	m_pHeapManager = new DXHeapManager(pContext, L"Model SRV", 2048);
}

DXModelManager::~DXModelManager()
{
	if (m_bIsSRVOwner)
		delete m_pHeapManager;
}

uint32_t DXModelManager::AddModel(Mapper* pMapper)
{
	uint32_t uiID = m_pHeapManager->ReserveResource();
	m_pMappers.emplace(uiID, std::unique_ptr<Mapper>(pMapper));

	// Can't exceed max count
	if (uiID >= m_pHeapManager->GetMaxCount())
	{
		return uiID;
	}

	const Mapper::Info& mapperInfo = pMapper->GetInfo();

	// Create SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC rd = {};
	rd.Format = static_cast<DXGI_FORMAT>(mapperInfo.m_ColorFormat);
	rd.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	rd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	rd.Buffer.NumElements = mapperInfo.m_uiElementCount;
	rd.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	if (mapperInfo.m_ColorFormat == E_UNKNOWN)
	{
		rd.Buffer.StructureByteStride = mapperInfo.m_uiElementSize;
	}

	m_pContext->GetDevice()->CreateShaderResourceView(pMapper->GetNative(), &rd, m_pHeapManager->GetCPUHandle(uiID));

	return uiID;
}

void DXModelManager::DestroyModel(uint32_t uiID)
{
	auto it = m_pMappers.find(uiID);

	if (it != m_pMappers.end())
	{
		it->second.reset();
		m_pMappers.erase(it);

		m_pHeapManager->FreeResource(uiID);
	}
}
