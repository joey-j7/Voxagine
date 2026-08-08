#include "VKUploadBuffer.h"

#include "VKAllocator.h"
#include "VKDevice.h"

#include <cstdio>

namespace
{
	size_t AlignUp(size_t uiValue, size_t uiAlignment)
	{
		return (uiValue + uiAlignment - 1) & ~(uiAlignment - 1);
	}
}

VKUploadBuffer::Page::Page(VKDevice* pDevice, const VKAllocator* pAllocator, size_t uiSizeInBytes)
	: m_uiPageSize(uiSizeInBytes)
{
	const bool bCreated = m_Resource.CreateBuffer(
		pDevice, pAllocator, uiSizeInBytes,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (!bCreated)
		return;

	m_Resource.SetDebugName("UploadPage");

	/* Upload pages stay mapped for their whole life; the DX12 version did the
	   same. Persistent mapping is cheap and avoids a map/unmap per allocation. */
	m_pCPU = m_Resource.Map();

	if (m_pCPU == nullptr)
		return;

	VkBufferDeviceAddressInfo addressInfo{};
	addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	addressInfo.buffer = m_Resource.GetBuffer();

	m_GPU = vkGetBufferDeviceAddress(pDevice->Get(), &addressInfo);
}

VKUploadBuffer::Page::~Page()
{
	m_Resource.Destroy();
}

bool VKUploadBuffer::Page::HasSpace(size_t uiSizeInBytes, size_t uiAlignment) const
{
	const size_t uiAlignedSize = AlignUp(uiSizeInBytes, uiAlignment);
	const size_t uiAlignedOffset = AlignUp(m_uiOffset, uiAlignment);

	return uiAlignedOffset + uiAlignedSize <= m_uiPageSize;
}

VKUploadBuffer::Allocation VKUploadBuffer::Page::Allocate(size_t uiSizeInBytes, size_t uiAlignment)
{
	Allocation allocation;

	if (!HasSpace(uiSizeInBytes, uiAlignment))
		return allocation;

	const size_t uiAlignedSize = AlignUp(uiSizeInBytes, uiAlignment);
	m_uiOffset = AlignUp(m_uiOffset, uiAlignment);

	allocation.CPU = static_cast<uint8_t*>(m_pCPU) + m_uiOffset;
	allocation.GPU = m_GPU + m_uiOffset;
	allocation.Buffer = m_Resource.GetBuffer();
	allocation.Offset = m_uiOffset;

	m_uiOffset += uiAlignedSize;

	return allocation;
}

VKUploadBuffer::VKUploadBuffer(VKDevice* pDevice, const VKAllocator* pAllocator, size_t uiPageSize)
	: m_pDevice(pDevice)
	, m_pAllocator(pAllocator)
	, m_uiPageSize(uiPageSize)
{
	/* Constant buffer offsets must respect the device's minimum alignment;
	   D3D12 hardcoded 256, Vulkan reports it. */
	VkPhysicalDeviceProperties props{};
	vkGetPhysicalDeviceProperties(pDevice->GetPhysicalDevice(), &props);

	m_uiConstantAlignment = static_cast<size_t>(props.limits.minUniformBufferOffsetAlignment);

	if (m_uiConstantAlignment == 0)
		m_uiConstantAlignment = 256;
}

VKUploadBuffer::~VKUploadBuffer() = default;

VKUploadBuffer::Allocation VKUploadBuffer::AllocateConstant(size_t uiSizeInBytes)
{
	return Allocate(AlignUp(uiSizeInBytes, m_uiConstantAlignment), m_uiConstantAlignment);
}

VKUploadBuffer::Allocation VKUploadBuffer::Allocate(size_t uiSizeInBytes, size_t uiAlignment)
{
	if (uiSizeInBytes > m_uiPageSize)
	{
		fprintf(stderr, "[vulkan] upload allocation of %zu exceeds page size %zu\n",
		        uiSizeInBytes, m_uiPageSize);
		return Allocation{};
	}

	if (m_pCurrentPage == nullptr || !m_pCurrentPage->HasSpace(uiSizeInBytes, uiAlignment))
		m_pCurrentPage = RequestPage();

	if (m_pCurrentPage == nullptr)
		return Allocation{};

	return m_pCurrentPage->Allocate(uiSizeInBytes, uiAlignment);
}

std::shared_ptr<VKUploadBuffer::Page> VKUploadBuffer::RequestPage()
{
	std::shared_ptr<Page> pPage;

	if (!m_AvailablePages.empty())
	{
		pPage = m_AvailablePages.front();
		m_AvailablePages.pop_front();

		return pPage;
	}

	pPage = std::make_shared<Page>(m_pDevice, m_pAllocator, m_uiPageSize);

	if (!pPage->IsValid())
	{
		fprintf(stderr, "[vulkan] failed to create a %zu byte upload page\n", m_uiPageSize);
		return nullptr;
	}

	m_PagePool.push_back(pPage);

	return pPage;
}

void VKUploadBuffer::Reset()
{
	m_pCurrentPage = nullptr;
	m_AvailablePages = m_PagePool;

	for (std::shared_ptr<Page>& pPage : m_AvailablePages)
		pPage->Reset();
}
