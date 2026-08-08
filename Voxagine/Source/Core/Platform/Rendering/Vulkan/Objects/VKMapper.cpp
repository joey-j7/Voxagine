#include "pch.h"

#include "Core/Platform/Rendering/Objects/Mapper.h"

#include "Core/Platform/Rendering/Objects/View.h"
#include "Core/Platform/Rendering/RenderPass.h"
#include "Core/Platform/Rendering/Vulkan/VKAllocator.h"
#include "Core/Platform/Rendering/Vulkan/VKRenderContext.h"

#include <cstring>

/* Host-visible storage buffers the CPU writes into every frame - voxel data,
 * particles, sprite lists.
 *
 * D3D12 used a CUSTOM heap with WRITE_BACK/L0 so one resource was both mapped
 * and GPU-readable. HOST_VISIBLE | HOST_COHERENT is the Vulkan equivalent, and
 * on discrete GPUs both are really writes across PCIe.
 *
 * The DX12 CreateView overloads wrote an SRV or UAV into a pass's heap slot.
 * Vulkan descriptors are written per pass at bind time, so those collapse to
 * bookkeeping and VKRenderPass reads the buffer straight off the Mapper. */

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

	m_pMapper[0].reset();
	m_pMapper[1].reset();
}

bool Mapper::Resize(uint32_t uiElementCount, uint32_t uiElementSize)
{
	if (uiElementCount == m_Info.m_uiElementCount && uiElementSize == m_Info.m_uiElementSize)
		return false;

	m_Info.m_uiElementCount = uiElementCount;
	m_Info.m_uiElementSize = uiElementSize;

	const VkDeviceSize uiBytes =
		static_cast<VkDeviceSize>(m_Info.m_uiElementCount) * m_Info.m_uiElementSize;

	if (uiBytes == 0)
		return false;

	if (m_pMapper[0])
	{
		Unmap();

		m_pMapper[0].reset();
		m_pMapper[1].reset();
	}

	/* A mapper with a colour format is bound as a texel buffer, which needs
	   its own usage bits - creating a VkBufferView over a buffer without them
	   is invalid. Both are requested because the format decides which is used
	   and it is cheap to allow either. */
	VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
	                           VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
	                           VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |
	                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
	                           VK_BUFFER_USAGE_TRANSFER_DST_BIT |
	                           VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	const uint32_t uiCount = m_Info.m_bHasBackBuffer ? 2u : 1u;

	/* Prefer memory the GPU reads at full speed and the CPU can still write
	   through - the resizable-BAR heap. Without it these buffers live in
	   system memory and every marcher fetch is a PCIe read; the voxel window
	   alone is 288 MiB per buffer and is read tens of times per pixel.
	   The heap is small or absent on older hardware, and even where it exists
	   it may not have room, so both the existence test and the allocation
	   itself fall back. CPU *reads* stay slow either way: both are uncached. */
	const VkMemoryPropertyFlags hostVisible =
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	const bool bHasDeviceLocalHostVisible =
		m_pContext->GetAllocator()->FindMemoryType(
			UINT32_MAX, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | hostVisible) != UINT32_MAX;

	for (uint32_t i = 0; i < uiCount; ++i)
	{
		m_pMapper[i] = std::make_shared<VKResource>();

		const bool bCreated =
			(bHasDeviceLocalHostVisible &&
			 m_pMapper[i]->CreateBuffer(
				m_pContext->GetDevice(), m_pContext->GetAllocator(), uiBytes, usage,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | hostVisible)) ||
			m_pMapper[i]->CreateBuffer(
				m_pContext->GetDevice(), m_pContext->GetAllocator(), uiBytes, usage,
				hostVisible);

		if (!bCreated)
		{
			m_pMapper[i].reset();
			return false;
		}

		m_pMapper[i]->SetDebugName(m_Info.m_Name);
	}

	for (auto& target : m_mRenderPasses)
		CreateView(target.first, target.second);

	for (auto& target : m_mComputePasses)
		CreateView(target.first, target.second);

	Map();

	if (m_pData[0] != nullptr)
		memset(m_pData[0], 0, uiBytes);

	if (m_Info.m_bHasBackBuffer && m_pData[1] != nullptr)
		memset(m_pData[1], 0, uiBytes);

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

	if (!m_pMapper[0])
		return;

	/* Stays mapped for the resource's life, as the D3D12 version did. */
	m_pData[0] = static_cast<uint32_t*>(m_pMapper[0]->Map());

	if (m_Info.m_bHasBackBuffer && m_pMapper[1])
		m_pData[1] = static_cast<uint32_t*>(m_pMapper[1]->Map());

	m_bIsMapped = true;
}

void Mapper::Unmap()
{
	if (!m_bIsMapped)
		return;

	if (m_pMapper[0])
		m_pMapper[0]->Unmap();

	if (m_Info.m_bHasBackBuffer && m_pMapper[1])
		m_pMapper[1]->Unmap();

	m_pData[0] = nullptr;
	m_pData[1] = nullptr;

	m_bIsMapped = false;
}

void Mapper::AddTarget(PComputePass* pComputePass, uint32_t uiID)
{
	m_mComputePasses.emplace(pComputePass, uiID);
	CreateView(pComputePass, uiID);
}

void Mapper::AddTarget(PRenderPass* pRenderPass, uint32_t uiID)
{
	m_mRenderPasses.emplace(pRenderPass, uiID);
	CreateView(pRenderPass, uiID);
}

void Mapper::CreateView(PComputePass* pComputePass, uint32_t uiID)
{
	/* Descriptors are written when the pass binds; see VKRenderPass. */
	VX_UNUSED(pComputePass);
	VX_UNUSED(uiID);
}

void Mapper::CreateView(PRenderPass* pRenderPass, uint32_t uiID)
{
	VX_UNUSED(pRenderPass);
	VX_UNUSED(uiID);
}

void Mapper::CreateView(uint64_t uiGPUAddress, PResource* pMapper)
{
	VX_UNUSED(uiGPUAddress);
	VX_UNUSED(pMapper);
}
