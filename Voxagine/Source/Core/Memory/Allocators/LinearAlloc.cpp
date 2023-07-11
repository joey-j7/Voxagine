#include "pch.h"


#include "LinearAlloc.h"


int LinearAlloc::Initialize(const size_t bufferSizeInBytes, const MemoryType type, const int memoryProtection)
{
	int32_t ret;

	ret = Destroy();
	if (ret != SCE_OK)
		return ret;

	m_pBaseAddr = static_cast<uint8_t*>(
		AllocMem(
			bufferSizeInBytes,
			type,
			memoryProtection
			)
		);

	m_bufferSize = bufferSizeInBytes;

	m_pFront = m_pBaseAddr;

	return SCE_OK;
}

int LinearAlloc::Initialize(BaseAlloc* parent, const size_t bufferSizeInBytes)
{
	assert(parent && "[FreeListAlloc] Parent ptr was null!");

	int32_t ret = -1;

	ret = Destroy();
	if (ret != SCE_OK)
		return ret;

	m_pParent = parent;
	m_pBaseAddr = static_cast<uint8_t*>(parent->Allocate(bufferSizeInBytes, 32));
	m_bufferSize = bufferSizeInBytes;

	m_pFront = m_pBaseAddr;

	if (m_pBaseAddr)
		return SCE_OK;
	return -1;
}

int LinearAlloc::Destroy()
{
	if (m_pBaseAddr)
	{
		if (m_pParent)
		{
			m_pParent->Free(m_pBaseAddr);
		}
		else
		{
			FreeMem(m_pBaseAddr);
		}
	}
		

	m_pBaseAddr = nullptr;
	m_pFront = nullptr;
	m_bufferSize = 0;

	return SCE_OK;

}

void* LinearAlloc::Allocate(size_t size, size_t align)
{
	assert(m_pBaseAddr && "[LinearAlloc] m_BaseAddr is null");

	const size_t adjustment = alignAdjustment(m_pBaseAddr + m_bufferSize, align);
	if (reinterpret_cast<intptr_t>(m_pFront) + adjustment + size > reinterpret_cast<intptr_t>(m_pBaseAddr) + m_bufferSize)
	{
		printf("[LinearAlloc][%p] Out of memory!\n", this);
		throw std::bad_alloc();
	}

	

#ifdef _DEBUG

	BlockHeader* headerPtr = reinterpret_cast<BlockHeader*>(m_pFront);
	*headerPtr = BlockHeader(size, adjustment);

	void* ptr = (m_pFront + sizeof(BlockHeader) + adjustment);

	BlockFooter* footerPtr = reinterpret_cast<BlockFooter*>(m_pFront + sizeof(BlockHeader) + adjustment + size);
	*footerPtr = BlockFooter();

	m_pFront += sizeof(BlockHeader) + adjustment + size + sizeof(BlockFooter);
	return ptr;

#else
	uint8_t* ptr = m_pFront;
	ptr += adjustment; // Turns out it is faster to adjust the newly constructed pointer separately from the front ptr
	m_pFront += adjustment + size;

	return ptr;
#endif // _DEBUG

}

void LinearAlloc::Free(void* ptr)
{
	assert(ptr && "Ptr was null!");

#ifdef _DEBUG

	assert(IsValidBlock(ptr) && 
		"Ptr was either already deleted or it had been written by an object bigger than its allocated size");

	InvalidateBlock(ptr);

#endif // _DEBUG

}

void LinearAlloc::Clear()
{
	m_pFront = m_pBaseAddr;
}
