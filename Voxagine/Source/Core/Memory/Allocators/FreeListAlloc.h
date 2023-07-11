#pragma once

#include <assert.h> 
#include <stdio.h>
#include <stdint.h>

#include "BaseAlloc.h"

class FreeListAlloc : public BaseAlloc
{
public:

	virtual int Initialize(
		const size_t BufferSizeInBytes,
		const MemoryType type = ONION,
		const int memoryProtection = SCE_KERNEL_PROT_CPU_RW) override;
	
	virtual int Initialize(BaseAlloc* parent, const size_t BufferSizeInBytes) override;

	virtual int Destroy() override;

	virtual void* Allocate(size_t size, size_t align) override;
	
	virtual void Free(void* ptr) override;

	virtual bool HasBaseAddr() override 
	{
		return m_pBlock != nullptr;
	};

private:

	struct FreeListNode
	{
		size_t m_size = 0;
		FreeListNode* m_pNext = nullptr;
	};


	uint8_t* m_pBlock;
	size_t m_blockSize;
	FreeListNode* m_pFreeListRoot;
	FreeListNode* m_pFreeListHead;
	static const size_t s_minimalBufferSize = 64;

};



