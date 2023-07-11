#include "pch.h"
#include "DXTextureManager.h"

#include "Core/Resources/Formats/TextureReference.h"

#include "Core/Platform/Rendering/RenderContextInc.h"
#include "Core/Platform/Rendering/CommandEngineInc.h"

#include "Core/Platform/Rendering/DX12/DXHelper.h"

#include "Core/Platform/Rendering/DX12/Managers/DXHeapManager.h"

DXTextureManager::DXTextureManager(DX12RenderContext* pContext, DXHeapManager* pManager) : TextureManager(pContext)
{
	if (pManager)
	{
		m_pHeapManager = pManager;
		return;
	}

	m_bIsSRVOwner = true;
	m_pHeapManager = new DXHeapManager(pContext, L"Texture SRV", 256);
}

DXTextureManager::~DXTextureManager()
{
	if (m_bIsSRVOwner)
		delete m_pHeapManager;
}

uint32_t DXTextureManager::CreateTexture(CommandEngine* pEngine, const std::string& sName, uint8_t* pData, UVector2 uSize)
{
	uint32_t uiID = m_pHeapManager->ReserveResource();

	// Can't exceed max count
	if (uiID >= m_pHeapManager->GetMaxCount())
	{
		return uiID;
	}

	/* View */
	View::Info viewInfo;
	viewInfo.m_ColorFormat = E_R8G8B8A8_UNORM;
	viewInfo.m_DimensionType = E_TEXTURE_2D;
	viewInfo.m_Name = sName;
	viewInfo.m_State = E_STATE_COPY_DEST;
	viewInfo.m_Size = UVector3(uSize.x, uSize.y, 1);

	m_pViews.emplace(uiID, std::make_unique<View>(m_pContext->Get(), viewInfo));
	View* pView = m_pViews[uiID].get();

	// Create temporary upload heap
	Microsoft::WRL::ComPtr<ID3D12Resource> pMapper;

	uint32_t uiXAlignment = Align(viewInfo.m_Size.x, 256);

	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
		uiXAlignment * viewInfo.m_Size.y * sizeof(uint32_t)
	);

	ID3D12Device* pDevice = m_pContext->Get()->GetDevice();

	ThrowIfFailed(
		pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&pMapper)
		)
	);

	// Fill mapper data
	uint32_t* pMappedData = nullptr;

	pMapper->Map(0, nullptr, reinterpret_cast<void**>(&pMappedData));

	if (uiXAlignment != viewInfo.m_Size.x)
	{
		for (uint32_t y = 0; y < viewInfo.m_Size.y; ++y)
		{
			std::memcpy(&pMappedData[y * uiXAlignment], &pData[y * viewInfo.m_Size.x * sizeof(uint32_t)], viewInfo.m_Size.x * sizeof(uint32_t));
		}
	}
	else
	{
		std::memcpy(pMappedData, pData, viewInfo.m_Size.x * viewInfo.m_Size.y * sizeof(uint32_t));
	}

	pMapper->Unmap(0, nullptr);

	// Start command list
	bool bBeforeState = pEngine->IsStarted();
	pEngine->Start();

	// Copy mapper data to view
	D3D12_TEXTURE_COPY_LOCATION dstLoc = {
		pView->GetNative(),
		D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		0
	};

	UINT64 textureUploadBufferSize;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT textureLayout;

	auto desc = pView->GetNative()->GetDesc();
	desc.Width = uiXAlignment;
	m_pContext->Get()->GetDevice()->GetCopyableFootprints(&desc, 0, 1, 0, &textureLayout, nullptr, nullptr, &textureUploadBufferSize);

	D3D12_TEXTURE_COPY_LOCATION srcLoc = {
		pMapper.Get(),
		D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
		textureLayout
	};

	D3D12_BOX box = {
		0, 0, 0,
		viewInfo.m_Size.x,
		viewInfo.m_Size.y,
		1
	};

	pEngine->Get()->GetList()->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, &box);

	pEngine->Set(pView, E_STATE_PIXEL_SHADER_RESOURCE);
	pEngine->ApplyBarriers();
	pEngine->Execute();

	pEngine->WaitForGPU();

	if (bBeforeState)
	{
		pEngine->Reset();
		pEngine->Start();
	}

	// Create SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Format = static_cast<DXGI_FORMAT>(viewInfo.m_ColorFormat);
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	m_pContext->Get()->GetDevice()->CreateShaderResourceView(pView->GetNative(), &srvDesc, m_pHeapManager->GetCPUHandle(uiID));

	return uiID;
}

uint32_t DXTextureManager::CreateEmptyTexture()
{
	return m_pHeapManager->ReserveResource();
}

bool DXTextureManager::LoadTexture(CommandEngine* pEngine, TextureReference* pTextureReference)
{
	const std::string& texPath = pTextureReference->GetRefPath();
	TextureReadData* pTextureData = ReadTexture(texPath);

	if (!pTextureData->m_Data)
	{
		delete pTextureData;
		return false;
	}

	uint32_t uiID = CreateTexture(pEngine, texPath, reinterpret_cast<uint8_t*>(pTextureData->m_Data), pTextureData->m_Dimensions);
	delete pTextureData;

	if (uiID >= m_pHeapManager->GetMaxCount())
	{
		return false;
	}

	pTextureReference->m_uiID = uiID;
	pTextureReference->TextureView = m_pViews[uiID].get();

	D3D12_GPU_DESCRIPTOR_HANDLE* handle = new D3D12_GPU_DESCRIPTOR_HANDLE();
	*handle = m_pHeapManager->GetGPUHandle(uiID);
	pTextureReference->Descriptor = handle;

	return true;
}

void DXTextureManager::DestroyTexture(const TextureReference* pTextureReference)
{
	if (!pTextureReference->IsLoaded())
		return;

	m_pHeapManager->FreeResource(pTextureReference->GetID());
	TextureManager::DestroyTexture(pTextureReference);
}
