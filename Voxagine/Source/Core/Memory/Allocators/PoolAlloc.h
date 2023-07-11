#pragma once

#include "BaseAlloc.h"

class PoolAlloc : public BaseAlloc
{
public:

	PoolAlloc() = delete;
	PoolAlloc(size_t objectSize, size_t alignment );

	virtual int Initialize(
		const size_t bufferSizeInBytes,
		const MemoryType type = ONION,
		const int memoryProtection = SCE_KERNEL_PROT_CPU_RW) override;

	int Initialize(BaseAlloc* parent, const size_t bufferSizeInBytes) override;

	virtual int Destroy() override;

	virtual void* Allocate(size_t size, size_t align) override;
	virtual void Free(void* ptr) override;

	virtual bool HasBaseAddr() override { return (m_pBaseAddr != nullptr); };

private:

	uint8_t* m_pBaseAddr;
	size_t m_bufferSize;
	size_t m_objectSize;
	size_t m_alignment;

	void** m_ppFreeListRoot;

};