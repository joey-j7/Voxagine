#include "pch.h"
#include "StackAlloc.h"

#include <assert.h>
#include <stdio.h>


StackAlloc::StackAlloc() :
	m_pBaseAddr(nullptr),
	m_bufferSize(0),
	m_pFront(nullptr)
{

}


int StackAlloc::Initialize(const size_t bufferSizeInBytes, const MemoryType type, const int memoryProtection )
{
	if (bufferSizeInBytes < s_mnimalBufferSize)
	{
		assert(bufferSizeInBytes >= s_mnimalBufferSize && "[StackAlloc] Buffer size too small!" );
		printf("[StackAlloc][%p] Buffer size too small!\n", this);
		return -1;
	}

	// Make sure this allocator has freed any previous allocated block
	Destroy();

	m_bufferSize = bufferSizeInBytes;

	m_pBaseAddr = static_cast<uint8_t*>(
		AllocMem(
			bufferSizeInBytes,
			type,
			memoryProtection
			) 
		);

	m_pFront = m_pBaseAddr;

	return SCE_OK;
}


int StackAlloc::Initialize(BaseAlloc* pParent, const size_t bufferSizeInBytes)
{
	assert(pParent && "[StackAlloc] Parent ptr was null!");
	m_pParent = pParent;
	m_pBaseAddr = static_cast<uint8_t*>(pParent->Allocate(bufferSizeInBytes, 32));
	m_bufferSize = bufferSizeInBytes;
	if (m_pBaseAddr)
		return SCE_OK;
	return -1;
}

int StackAlloc::Destroy()
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
		m_pBaseAddr = nullptr;
	}

	return SCE_OK;
}


void* StackAlloc::Allocate(size_t size, size_t align)
{
	assert(size != 0 && "[StackAlloc] Requested size to allocate is 0!");

	uint8_t* ptr = m_pFront;
	size_t adjustment = alignAdjustment( m_pFront + size + sizeof(StackHeader), align);
	ptr += adjustment;


#ifdef _DEBUG

	if (reinterpret_cast<intptr_t>(m_pFront) + adjustment + sizeof(StackHeader) + size + sizeof(BlockFooter) > 
		reinterpret_cast<intptr_t>(m_pBaseAddr) + m_bufferSize)
	{
		printf("[StackAlloc][%p] Out of memory!\n", this);
		throw std::bad_alloc();
	}

#else

	if (reinterpret_cast<intptr_t>(m_pFront) + adjustment + sizeof(StackHeader) + size > reinterpret_cast<intptr_t>(m_pBaseAddr) + m_bufferSize)
	{
		printf("[StackAlloc][%p] Out of memory!\n", this);
		throw std::bad_alloc();
	}

#endif // _DEBUG

	
	StackHeader* headerPtr = reinterpret_cast<StackHeader*>(ptr);
	headerPtr->m_alignAdjustment = adjustment;

#if _DEBUG

	headerPtr->m_size = size;
	headerPtr->m_pPrev = m_pPrev;

	BlockFooter* footerPtr = reinterpret_cast<BlockFooter*>(reinterpret_cast<uint8_t*>(headerPtr) + sizeof(StackHeader) + size );
	*footerPtr = BlockFooter();

	m_pPrev = ptr = reinterpret_cast<uint8_t*>(headerPtr) + sizeof(StackHeader);

	// Update the front ptr
	m_pFront = reinterpret_cast<uint8_t*>(footerPtr) + sizeof(BlockFooter);

	return ptr;

#else

	ptr = reinterpret_cast<uint8_t*>(headerPtr) + sizeof(StackHeader);

	m_pFront = ptr + size;

	return ptr;

#endif
}

void StackAlloc::Free(void* ptr)
{
	assert(ptr && "Delete was passed a nullptr!");

	uint8_t* blockPtr = static_cast<uint8_t*>(ptr);
	StackHeader* headerPtr = reinterpret_cast<StackHeader*>(blockPtr - sizeof(StackHeader));

#ifdef _DEBUG

	assert(IsValidBlock<StackHeader>(ptr) &&
		"Ptr was either already deleted or it had been written by an object bigger than its allocated size");

	InvalidateBlock<StackHeader>(ptr);

	assert(ptr == m_pPrev && "[StackAlloc] Not used as a stack!");
	m_pPrev = headerPtr->m_pPrev;
#endif

	m_pFront = static_cast<uint8_t*>(ptr) - (headerPtr->m_alignAdjustment + sizeof(StackHeader));

}
