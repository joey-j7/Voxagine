#include "pch.h"
#include "PoolAlloc.h"

#include <assert.h>

PoolAlloc::PoolAlloc(size_t objectSize, size_t alignment) :
	m_pBaseAddr(nullptr),
	m_bufferSize(0),
	m_objectSize(objectSize),
	m_alignment(alignment),
	m_ppFreeListRoot(nullptr)
{
	assert( m_objectSize >= sizeof(void*) && "Object size needs to be at least the size of a void*" );

#ifdef _DEBUG

	m_objectSize += sizeof(BlockHeader) + sizeof(BlockFooter);

#endif // _DEBUG

}

int PoolAlloc::Initialize(const size_t bufferSizeInBytes, const MemoryType type, const int memoryProtection )
{
	assert(bufferSizeInBytes > m_objectSize && "Buffer too small!");

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

	// Align the first block and the rest will be aligned as well on initialization
	m_ppFreeListRoot = reinterpret_cast<void**>( alignPtrTo(m_pBaseAddr, m_alignment) );

	void** ptr = m_ppFreeListRoot;
	size_t maxAmountOfObject = m_bufferSize / m_objectSize - 1;

	// Initialize the free list pointers
	for( size_t i = 0; i < maxAmountOfObject; i++ )
	{
		*ptr = ( reinterpret_cast<uint8_t*>(ptr) + m_objectSize );
		ptr = reinterpret_cast<void**>(*ptr);
	}

	// End of the list
	*ptr = nullptr;

	return SCE_OK;
}

int PoolAlloc::Initialize(BaseAlloc* pParent, const size_t bufferSizeInBytes)
{
	assert(pParent && "[StackAlloc] pParent ptr was null!");
	m_pParent = pParent;
	m_pBaseAddr = static_cast<uint8_t*>(pParent->Allocate(bufferSizeInBytes, 32));
	m_bufferSize = bufferSizeInBytes;
	if(m_pBaseAddr)
		return SCE_OK;
	return -1;
}

int PoolAlloc::Destroy()
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

	m_ppFreeListRoot = nullptr;

	return SCE_OK;
}

void* PoolAlloc::Allocate(size_t size, size_t align)
{
	assert( sizeof(BlockHeader) + size + sizeof(BlockFooter) <= m_objectSize && align == m_alignment );

	if (m_ppFreeListRoot == nullptr)
	{
		printf("[PoolAlloc][%p] Out of memory!\n", this);
		throw std::bad_alloc();
	}

	uint8_t* ptr = reinterpret_cast<uint8_t*>( m_ppFreeListRoot );
	m_ppFreeListRoot = reinterpret_cast<void**>(*m_ppFreeListRoot);

#ifdef _DEBUG

	BlockHeader* headerPtr = reinterpret_cast<BlockHeader*>(ptr);
	*headerPtr = BlockHeader( size, 0 );

	ptr += sizeof(BlockHeader);

	BlockFooter* footerPtr = reinterpret_cast<BlockFooter*>( ptr + size );
	*footerPtr = BlockFooter();

#endif
	
	return ptr;
}

void PoolAlloc::Free(void* ptr)
{
	assert(ptr && "Delete was passed a null ptr!");

#ifdef _DEBUG

	assert(IsValidBlock(ptr) &&
		"Ptr was either already deleted or it had been written by an object bigger than its allocated size");

	InvalidateBlock(ptr);

#endif // _DEBUG

	*(reinterpret_cast<void**>(ptr)) = m_ppFreeListRoot;
	m_ppFreeListRoot = reinterpret_cast<void**>(ptr);
}

