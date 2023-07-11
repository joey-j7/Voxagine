#include "pch.h"
#include "Buffer.h"

void Buffer::AddStructuredData(void* pData, size_t instanceSize, size_t instanceCount, bool bCopy)
{
	m_uiTotalSize += static_cast<uint32_t>(instanceSize * instanceCount);
	m_uiInstanceCount += static_cast<uint32_t>(instanceCount);

	// Set strides to buffer's structure
	// assert(m_uiTotalStrides == 0 || m_uiTotalStrides == buffer->GetStrides().size());
	// m_uiTotalStrides = static_cast<uint32_t>(buffer->GetStrides().size());
	// m_Strides = buffer->GetStrides();

	// Set total size to buffer's structure
	assert(m_uiInstanceSize == 0 || m_uiInstanceSize == instanceSize);
	m_uiInstanceSize = static_cast<uint32_t>(instanceSize);

	if (bCopy)
	{
		// Append data to own array
		if (m_CopiedData.size() < m_uiTotalSize)
		{
			m_CopiedData.resize(m_uiTotalSize);
		}

		std::memcpy(
			&m_CopiedData[m_uiTotalSize - m_uiInstanceSize * instanceCount],
			reinterpret_cast<const uint8_t*>(pData),
			m_uiInstanceSize * instanceCount
		);

		m_pData = m_CopiedData.data();
	}
	else
	{
		m_pData = reinterpret_cast<uint8_t*>(pData);
	}
}

void Buffer::Clear()
{
	// We don't necessarily need to clear these, resizing might be expensive, meaning that the
	// biggest data instance determines the buffer's size eventually
	if (m_Info.m_ClearDataEveryFrame)
	{
		m_CopiedData.clear();
		m_Strides.clear();
	}

	m_uiTotalSize = 0;
	m_uiInstanceCount = 0;
	m_uiTotalStrides = 0;
	m_uiInstanceSize = 0;
}
