#include "pch.h"
#include "DXHeapManager.h"

#include "Core/Platform/Rendering/RenderContextInc.h"
#include "Core/Platform/Rendering/DX12/DXHelper.h"

DXHeapManager::DXHeapManager(PRenderContext* pContext, const std::wstring& sName, uint32_t uiMaxCount) : IDManager(pContext, uiMaxCount)
{
	uint32_t uiMinCount = 1;
	uiMaxCount = std::max(uiMaxCount, uiMinCount);

	m_uiMaxCount = uiMaxCount;

	// Reserve SRV handle
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NumDescriptors = uiMaxCount;

	ID3D12Device* pDevice = m_pContext->GetDevice();

	// Describe and create a shader resource view (SRV) heap for the texture.
	ThrowIfFailed(pDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pDescriptorHeap)));
	m_uiDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_pDescriptorHeap->SetName(sName.c_str());
}

D3D12_CPU_DESCRIPTOR_HANDLE DXHeapManager::GetCPUHandle(uint32_t uiID)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), uiID, m_uiDescriptorSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE DXHeapManager::GetGPUHandle(uint32_t uiID)
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), uiID, m_uiDescriptorSize);
}
