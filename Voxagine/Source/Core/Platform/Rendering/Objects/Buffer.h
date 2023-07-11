#pragma once

#include <vector>
#include <stdint.h>

#include "Math.h"

#include "Core/Platform/Rendering/RenderDefines.h"

class Buffer
{
	friend class DepthPass;

public:
	enum Type
	{
		E_CONSTANT,
		E_STRUCTURED,
		E_VERTEX,
		E_INDEX
	};

	struct Info
	{
		GPUAccessType	m_GPUAccessType = E_READ_ONLY;

		Type			m_Type = E_CONSTANT;
		std::string		m_Name = "Unnamed";
		bool			m_ClearDataEveryFrame = false;
	};

	Buffer(PRenderContext* pContext, const Info& info);

	template <typename T>
	void AddConstantData(const T& data);

	void AddStructuredData(void* pData, size_t instanceSize, size_t uiInstances, bool bCopy = true);

	void Clear();

	void Allocate(bool bMove = true);

	const std::vector<uint32_t>& GetStrides() const { return m_Strides; }
	uint32_t GetTotalSize() const { return m_uiTotalSize; }

	const Info& GetInfo() const { return m_Info; }
	PUploadBuffer* GetNative() const { return m_pNative.get(); }
	uint64_t GetGPUAddress() const { return m_uiGPUAddress; }

	uint32_t GetInstanceCount() const { return m_uiInstanceCount; }

protected:
	PRenderContext* m_pContext = nullptr;
	Info m_Info;

	std::vector<uint8_t> m_CopiedData;
	std::vector<uint32_t> m_Strides;

	uint8_t* m_pData;

	uint32_t m_uiTotalSize = 0;
	uint32_t m_uiLastTotalSize = 0;

	uint32_t m_uiTotalStrides = 0;
	uint32_t m_uiInstanceCount = 0;
	uint32_t m_uiInstanceSize = 0;

	uint8_t* m_pCPUAddress = nullptr;
	uint64_t m_uiGPUAddress = 0;

	std::unique_ptr<PUploadBuffer> m_pNative;
};

template <typename T>
void Buffer::AddConstantData(const T& data)
{
	uint32_t dataSize = static_cast<uint32_t>(sizeof(T));
	m_uiTotalSize += dataSize;
	m_uiInstanceSize = m_uiTotalSize;
	m_uiTotalStrides++;

	if (m_CopiedData.size() < m_uiTotalSize)
	{
		m_CopiedData.resize(m_uiTotalSize);
	}

	// Strides for constant buffers never change
	if (m_Strides.size() < m_uiTotalStrides)
	{
		m_Strides.push_back(dataSize);
	}

	std::memcpy(&m_CopiedData[m_uiTotalSize - dataSize], reinterpret_cast<const uint8_t*>(&data), dataSize);
	m_pData = m_CopiedData.data();

	// We now have one instance of a constant buffer
	m_uiInstanceCount = 1;
}