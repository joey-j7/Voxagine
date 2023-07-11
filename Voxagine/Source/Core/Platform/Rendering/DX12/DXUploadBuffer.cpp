#include "pch.h"
#include "DXUploadBuffer.h"

#include "DXHelper.h"

#include "External/DirectX12/d3dx12.h"

DXUploadBuffer::DXUploadBuffer(ID3D12Device* device, size_t pageSize)
	: m_PageSize(pageSize)
{
	m_pDevice = device;
}

DXUploadBuffer::~DXUploadBuffer()
{}

DXUploadBuffer::Allocation DXUploadBuffer::Allocate(size_t sizeInBytes, size_t alignment)
{
	if (sizeInBytes > m_PageSize)
	{
		throw std::bad_alloc();
	}

	// If there is no current page, or the requested allocation exceeds the
	// remaining space in the current page, request a new page.
	if (!m_CurrentPage || !m_CurrentPage->HasSpace(sizeInBytes, alignment))
	{
		m_CurrentPage = RequestPage();
	}

	return m_CurrentPage->Allocate(sizeInBytes, alignment);
}

std::shared_ptr<DXUploadBuffer::Page> DXUploadBuffer::RequestPage()
{
	std::shared_ptr<Page> page;

	if (!m_AvailablePages.empty())
	{
		page = m_AvailablePages.front();
		m_AvailablePages.pop_front();
	}
	else
	{
		page = std::make_shared<Page>(m_PageSize);
		page->Map(m_pDevice);
		m_PagePool.push_back(page);
	}

	return page;
}

void DXUploadBuffer::Reset()
{
	m_CurrentPage = nullptr;
	// Reset all available pages.
	m_AvailablePages = m_PagePool;

	for (auto page : m_AvailablePages)
	{
		// Reset the page for new allocations.
		page->Reset();
	}
}

DXUploadBuffer::Page::Page(size_t sizeInBytes)
	: m_PageSize(sizeInBytes)
	, m_Offset(0)
	, m_CPUPtr(nullptr)
	, m_GPUPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{

}

DXUploadBuffer::Page::~Page()
{
	m_d3d12Resource->Unmap(0, nullptr);
	m_CPUPtr = nullptr;
	m_GPUPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

void DXUploadBuffer::Page::Map(ID3D12Device* device)
{
	auto uploadBuffer = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto pageSize = CD3DX12_RESOURCE_DESC::Buffer(m_PageSize);

	ThrowIfFailed(device->CreateCommittedResource(
		&uploadBuffer,
		D3D12_HEAP_FLAG_NONE,
		&pageSize,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_d3d12Resource)
	));

	m_GPUPtr = m_d3d12Resource->GetGPUVirtualAddress();
	m_d3d12Resource->Map(0, nullptr, &m_CPUPtr);
}

bool DXUploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
	size_t alignedSize = ((size_t)sizeInBytes & ~(alignment - 1));
	size_t alignedOffset = ((size_t)m_Offset & ~(alignment - 1));

	return alignedOffset + alignedSize <= m_PageSize;
}

DXUploadBuffer::Allocation DXUploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
	if (!HasSpace(sizeInBytes, alignment))
	{
		// Can't allocate space from page.
		throw std::bad_alloc();
	}

	size_t alignedSize = ((size_t)sizeInBytes & ~(alignment - 1));
	m_Offset = ((size_t)m_Offset & ~(alignment - 1));

	Allocation allocation;
	allocation.CPU = static_cast<uint8_t*>(m_CPUPtr) + m_Offset;
	allocation.GPU = m_GPUPtr + m_Offset;

	m_Offset += alignedSize;

	return allocation;
}

void DXUploadBuffer::Page::Reset()
{
	m_Offset = 0;
}