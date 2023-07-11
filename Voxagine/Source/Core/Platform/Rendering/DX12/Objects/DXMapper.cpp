#include "pch.h"
#include "Core/Platform/Rendering/Objects/Mapper.h"

#include "Core/Platform/Rendering/Objects/View.h"

#include "Core/Platform/Rendering/DX12/DX12RenderContext.h"
#include "Core/Platform/Rendering/DX12/DXHelper.h"
#include "Core/Platform/Rendering/DX12/DXCommandEngine.h"
#include "Core/Platform/Rendering/DX12/DXRenderPass.h"

#include "Core/Platform/Rendering/RenderPass.h"
#include "Core/Platform/Rendering/DX12/Managers/DXHeapManager.h"

#include <codecvt>
#include <xlocbuf>

Mapper::Mapper(PRenderContext* pContext, const Info& info, bool bCreate)
{
	m_pContext = pContext;
	m_Info = info;

	if (bCreate)
	{
		// Trigger creation in resize function
		uint32_t count = m_Info.m_uiElementCount;
		uint32_t size = m_Info.m_uiElementSize;

		m_Info.m_uiElementCount = 0;
		m_Info.m_uiElementSize = 0;

		Resize(count, size);
	}
}

Mapper::~Mapper()
{
	Unmap();
}

bool Mapper::Resize(uint32_t uiElementCount, uint32_t uiElementSize)
{
	if (uiElementCount == m_Info.m_uiElementCount && uiElementSize == m_Info.m_uiElementSize)
		return false;

	m_Info.m_uiElementCount = uiElementCount;
	m_Info.m_uiElementSize = uiElementSize;
	
	/*CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(
		m_Info.m_GPUAccessType == E_READ_WRITE ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD
	);*/

	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
		m_Info.m_uiElementCount * m_Info.m_uiElementSize,
		m_Info.m_GPUAccessType == E_READ_WRITE ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE
	);
	
	// Describe the heap properties
	D3D12_HEAP_PROPERTIES heapProps = {};

	heapProps.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	if (m_pMapper[0])
	{
		Unmap();
		m_pMapper[0].Reset();

		if (m_pMapper[1])
		{
			m_pMapper[1].Reset();
		}
	}

	ID3D12Device* pDevice = m_pContext->GetDevice();
	
	for (uint32_t i = 0; i < (m_Info.m_bHasBackBuffer ? 2u : 1u); ++i)
	{
		ThrowIfFailed(
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&bufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_pMapper[i])
			)
		);

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring nameW = converter.from_bytes(m_Info.m_Name);
		m_pMapper[i]->SetName(nameW.c_str());
	}

	for (auto& target : m_mRenderPasses)
	{
		CreateView(target.first, target.second);
	}

	for (auto& target : m_mComputePasses)
	{
		CreateView(target.first, target.second);
	}
	
	Map();
	memset(m_pData[0], 0, m_Info.m_uiElementCount * m_Info.m_uiElementSize);

	if (m_Info.m_bHasBackBuffer)
	{
		memset(m_pData[1], 0, m_Info.m_uiElementCount * m_Info.m_uiElementSize);
	}

	return true;
}

void Mapper::SwapBuffer()
{
	if (!m_Info.m_bHasBackBuffer)
		return;

	m_uiCurrentBackBuffer = (m_uiCurrentBackBuffer + 1) % 2;

	BufferSwapped(GetData());
}

void Mapper::Map()
{
	if (m_bIsMapped)
		return;

	/* Map voxel texture so the CPU can write to it */
	ThrowIfFailed(m_pMapper[0]->Map(0, nullptr, reinterpret_cast<void**>(&m_pData[0])));

	if (m_Info.m_bHasBackBuffer)
	{
		ThrowIfFailed(m_pMapper[1]->Map(0, nullptr, reinterpret_cast<void**>(&m_pData[1])));
	}

	m_bIsMapped = true;
}

void Mapper::Unmap()
{
	if (!m_bIsMapped)
		return;
	
	m_pMapper[0]->Unmap(0, nullptr);

	if (m_Info.m_bHasBackBuffer)
	{
		m_pMapper[1]->Unmap(0, nullptr);
	}

	m_bIsMapped = false;
}

void Mapper::AddTarget(PComputePass* pComputePass, uint32_t uiID)
{
	m_mComputePasses.emplace(pComputePass, uiID);
	CreateView(pComputePass, uiID);
}

void Mapper::CreateView(PComputePass* pComputePass, uint32_t uiID)
{
	if (m_Info.m_uiElementCount * m_Info.m_uiElementSize == 0 || m_Info.m_ColorFormat == E_UNKNOWN)
		return;

	D3D12_CPU_DESCRIPTOR_HANDLE handle = pComputePass->GetHeapManager()->GetCPUHandle(uiID);
	CreateView(handle.ptr, m_pMapper[0].Get());

	if (m_Info.m_bHasBackBuffer)
	{
		handle = pComputePass->GetHeapManager()->GetCPUHandle(uiID + 1);
		CreateView(handle.ptr, m_pMapper[1].Get());
	}
}

void Mapper::AddTarget(PRenderPass* pRenderPass, uint32_t uiID)
{
	m_mRenderPasses.emplace(pRenderPass, uiID);
	CreateView(pRenderPass, uiID);
}

void Mapper::CreateView(PRenderPass* pRenderPass, uint32_t uiID)
{
	if (m_Info.m_uiElementCount * m_Info.m_uiElementSize == 0 || m_Info.m_ColorFormat == E_UNKNOWN)
		return;

	D3D12_CPU_DESCRIPTOR_HANDLE handle = pRenderPass->GetHeapManager()->GetCPUHandle(uiID);
	CreateView(handle.ptr, m_pMapper[0].Get());

	if (m_Info.m_bHasBackBuffer)
	{
		handle = pRenderPass->GetHeapManager()->GetCPUHandle(uiID + 1);
		CreateView(handle.ptr, m_pMapper[1].Get());
	}
}

void Mapper::CreateView(uint64_t uiGPUAddress, PResource* pMapper)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = uiGPUAddress;
	
	// Create SRV
	if (m_Info.m_GPUAccessType == E_READ_ONLY)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC rd = {};

		rd.Format = static_cast<DXGI_FORMAT>(m_Info.m_ColorFormat);
		rd.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		rd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		rd.Buffer.NumElements = m_Info.m_uiElementCount;
		rd.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		if (m_Info.m_ColorFormat == E_UNKNOWN)
		{
			rd.Buffer.StructureByteStride = m_Info.m_uiElementSize;
		}

		m_pContext->GetDevice()->CreateShaderResourceView(pMapper, &rd, handle);
	}
	else
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC rd = {};

		rd.Format = static_cast<DXGI_FORMAT>(m_Info.m_ColorFormat);
		rd.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		rd.Buffer.NumElements = m_Info.m_uiElementCount;
		rd.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		if (m_Info.m_ColorFormat == E_UNKNOWN)
		{
			rd.Buffer.StructureByteStride = m_Info.m_uiElementSize;
		}

		m_pContext->GetDevice()->CreateUnorderedAccessView(pMapper, nullptr, &rd, handle);
	}
}