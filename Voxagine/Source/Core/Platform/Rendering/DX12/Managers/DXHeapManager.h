#pragma once

#include "Core/Platform/Rendering/Managers/IDManager.h"

#include <d3d12.h>
#include <stdint.h>

#include <string>

#include <wrl/client.h>

#include "Core/Platform/Rendering/RenderDefines.h"

class DXHeapManager : public IDManager
{
public:
	DXHeapManager(PRenderContext* pContext, const std::wstring& sName, uint32_t uiMaxCount);

	ID3D12DescriptorHeap* GetNative() const { return m_pDescriptorHeap.Get(); }
	uint32_t GetDescriptorSize() const { return m_uiDescriptorSize; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t uiID);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t uiID);

	uint32_t ReserveResource() { return ReserveID(); }
	void FreeResource(uint32_t uiID) { FreeID(uiID); }

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap;
	uint32_t m_uiDescriptorSize = 0;
};