#pragma once

#include <stdint.h>
#include <type_traits>

#include "BaseAlloc.h"

class StackAlloc : public BaseAlloc
{
public:

	StackAlloc();

	virtual int Initialize(
		const size_t BufferSizeInBytes,
		const MemoryType type = ONION,
		const int memoryProtection = SCE_KERNEL_PROT_CPU_RW) override;

	virtual int Initialize(BaseAlloc* parent, const size_t BufferSizeInBytes) override;

	virtual int Destroy() override;

	virtual void* Allocate(size_t size, size_t align) override;
	virtual void Free(void* ptr) override;

	virtual bool HasBaseAddr() override { return (m_pBaseAddr != nullptr); };

	struct StackHeader : public BaseAlloc::BlockHeader
	{
#ifdef _DEBUG
		void* m_pPrev;
#endif
	};

private:

#ifdef _DEBUG
	void* m_pPrev = nullptr;
#endif

	uint8_t* m_pBaseAddr;
	uint8_t* m_pFront;
	size_t m_bufferSize;

	// in bytes
	static const size_t s_mnimalBufferSize = 64;

};


