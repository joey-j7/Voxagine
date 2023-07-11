#pragma once

#include "BaseAlloc.h"

class LinearAlloc : public BaseAlloc
{
public:

	virtual int Initialize(
		const size_t BufferSizeInBytes,
		const MemoryType type = ONION,
		const int memoryProtection = SCE_KERNEL_PROT_CPU_RW) override;

	virtual int Initialize(BaseAlloc* parent, const size_t BufferSizeInBytes) override;

	virtual int Destroy() override;

	virtual void* Allocate(size_t size, size_t align) override;

	// Does nothing, except for some validity checks on debug
	virtual void Free(void* ptr) override;

	void Clear();

	virtual bool HasBaseAddr() override
	{
		return (m_pBaseAddr != nullptr);
	};

private:

	uint8_t* m_pBaseAddr = nullptr;
	uint8_t* m_pFront;
	size_t m_bufferSize;

};



