#include "pch.h"

#include "Core/Platform/Rendering/Objects/Buffer.h"

#include "Core/Platform/Rendering/Vulkan/VKRenderContext.h"
#include "Core/Platform/Rendering/Vulkan/VKUploadBuffer.h"

#include <cstring>

Buffer::Buffer(PRenderContext* pContext, const Info& info)
{
	m_pContext = pContext;
	m_Info = info;

	m_pData = nullptr;

	m_pNative = std::make_unique<PUploadBuffer>(pContext->GetDevice(), pContext->GetAllocator());
}

void Buffer::Allocate(bool bMove)
{
	// Make sure alignment is correct
	assert(m_uiTotalSize % 16 == 0);

	if (m_uiTotalSize != m_uiLastTotalSize)
	{
		VKUploadBuffer::Allocation mem = {};

		m_pNative->Reset();

		switch (m_Info.m_Type)
		{
		case Buffer::Type::E_CONSTANT:		mem = m_pNative->AllocateConstant(m_uiTotalSize); break;
		case Buffer::Type::E_STRUCTURED:	mem = m_pNative->Allocate(m_uiTotalSize, m_uiInstanceSize); break;
		default:							assert(false); break;
		}

		m_pCPUAddress = static_cast<uint8_t*>(mem.CPU);
		m_uiGPUAddress = mem.GPU;

		m_uiNativeHandle = reinterpret_cast<uint64_t>(mem.Buffer);
		m_uiNativeOffset = static_cast<uint64_t>(mem.Offset);

		m_uiLastTotalSize = m_uiTotalSize;
	}

	if (m_pCPUAddress == nullptr || m_pData == nullptr)
		return;

	// Fill mapped data
	if (bMove)
	{
		std::memmove(m_pCPUAddress, m_pData, m_uiTotalSize);
	}
	else
	{
		std::memcpy(m_pCPUAddress, m_pData, m_uiTotalSize);
	}
}
