#include "pch.h"
#include "FreeListAlloc.h"
#include "BaseAlloc.h"
#include <exception>

int FreeListAlloc::Initialize(
	const size_t BufferSizeInBytes,
	const MemoryType type,
	const int memoryProtection)
{

	if (BufferSizeInBytes < s_minimalBufferSize )
	{
		assert(BufferSizeInBytes >= s_minimalBufferSize && "[FreeListAlloc] Buffer size too small!");
		printf("[FreeListAlloc][%p] Buffer size too small!\n", this);
		return -1;
	}

	m_pFreeListRoot = nullptr;
	m_pFreeListHead = nullptr;

	m_pBlock = static_cast<uint8_t*>(
		AllocMem(
			BufferSizeInBytes,
			type,
			memoryProtection
		)
		);

	if (m_pBlock)
	{

		FreeListNode node;
		node.m_pNext = nullptr;
		node.m_size = BufferSizeInBytes - sizeof(FreeListNode);
		m_pFreeListRoot = reinterpret_cast<FreeListNode*>(m_pBlock);
		*m_pFreeListRoot = node;
		m_pFreeListHead = m_pFreeListRoot;

		//TODO print in b, kb, mb 
		printf("[FreeListAlloc][%p] Initialized %.1f MB buffer\n", this, (double)(BufferSizeInBytes / (1024 * 1024 )));

		m_blockSize = BufferSizeInBytes;
		return SCE_OK;
	}

	printf("[FreeListAlloc][%p] Failed to initialize buffer!\n", this);

	m_blockSize = 0;
	return -1;
}

int FreeListAlloc::Initialize(BaseAlloc* parent, const size_t BufferSizeInBytes)
{
	assert(parent && "[FreeListAlloc] Parent ptr was null!");
	m_pParent = parent;
	m_pFreeListRoot = nullptr;
	m_pFreeListHead = nullptr;
	m_pBlock = static_cast<uint8_t*>( parent->Allocate(BufferSizeInBytes, 32) );
	m_blockSize = BufferSizeInBytes;
	
	if (m_pBlock)
	{

		FreeListNode node;
		node.m_pNext = nullptr;
		node.m_size = BufferSizeInBytes - sizeof(FreeListNode);
		m_pFreeListRoot = reinterpret_cast<FreeListNode*>(m_pBlock);
		*m_pFreeListRoot = node;
		m_pFreeListHead = m_pFreeListRoot;

		//TODO print in b, kb, mb 
		printf("[FreeListAlloc][%p] Initialized %.1f MB buffer\n", this, (double)(BufferSizeInBytes / (1024 * 1024)));

		m_blockSize = BufferSizeInBytes;
		return SCE_OK;
	}

	return -1;
}

int FreeListAlloc::Destroy()
{

	if (m_pParent)
	{
		m_pParent->Free(m_pBlock);
	}
	else
	{
		FreeMem(m_pBlock);
	}

	m_pBlock = nullptr;

	printf("[FreeListAlloc][%p] Destroyed block.\n", this);
	return SCE_OK;
}


void* FreeListAlloc::Allocate(size_t size, size_t aligment)
{
	uint8_t* ptr = nullptr;

	size_t adjustment = 0;

	// If root is valid we have nodes to take memory form
	if (m_pFreeListRoot)
	{
		//I'm using a singly linked list, to both current and next node I add my root to a temporary node so that I don't have to modify my traversing method
		FreeListNode tempNode;
		tempNode.m_pNext = m_pFreeListRoot;

		FreeListNode* node = &tempNode;
		do
		{
			// I might need to change the address the pointer is pointing to, so I'm taking a reference of a pointer to work with
			FreeListNode*& nextNode = node->m_pNext;

			// then the pointer that used to be pointing to a node, will become a block header pointer later on
			ptr = reinterpret_cast<uint8_t*>(nextNode);

			adjustment = alignAdjustment(ptr + sizeof(BlockHeader), aligment);

			// If the next node has a size that's bigger or equal to the size required...
			if (nextNode->m_size >= size + adjustment)
			{

				// I need to store some values before I start messing around with the pointers
				FreeListNode temp;
				temp.m_pNext = nextNode->m_pNext;
				temp.m_size = nextNode->m_size - size;

				// If the block of memory left in the node is bigger than the size it takes to store a node...
				if (temp.m_size > sizeof(FreeListNode))
				{
					temp.m_size -= sizeof(FreeListNode);

#ifdef _DEBUG
					// then I set nextNode to the address behind the footer, effectively rerouting my list
					nextNode = reinterpret_cast<FreeListNode*>(ptr + sizeof(BlockHeader) + size + sizeof(BlockFooter));
#else
					// then I set nextNode to the address behind the footer, effectively rerouting my list
					nextNode = reinterpret_cast<FreeListNode*>(ptr + sizeof(BlockHeader) + size);
#endif // _DEBUG

					//Copy temp's values to the new node
					*nextNode = temp;
				}
				else
				{
					// else add the leftover size to the block the user is asking
					size += temp.m_size;

					// If the next node is valid,
					if (temp.m_pNext)
					{
						// we'll make the current node point at it
						nextNode = temp.m_pNext;
					}
					else
					{
						// else we'll make it null
						nextNode = nullptr;
					}
				}

			}

			// If valid,
			if (node->m_pNext)
			{
				// go to next node
				node = nextNode;
			}

		} while (node->m_pNext && ptr == nullptr);

		//Update the root
		m_pFreeListRoot = tempNode.m_pNext;

	}

	// If ptr isn't valid, we either couldn't find a node big enough to fit the requested amount 
	// or all memory was given out already
	if (!ptr)
	{
		printf("[FreeListAlloc][%p] Bad alloc!\n", this);
		throw std::bad_alloc();
	}


	ptr += adjustment;

	// ptr becomes the BlockHeader
	BlockHeader* headerPtr = reinterpret_cast<BlockHeader*>(ptr);
	*headerPtr = BlockHeader( size, adjustment );

	// Move front
	ptr += sizeof(BlockHeader);

	// blockPtr will be returned to the user
	void* blockPtr = reinterpret_cast<void*>(ptr);
	// Move front
	ptr += size;
	
#ifdef _DEBUG

	// Add a footer at the end
	BlockFooter* footerPtr = reinterpret_cast<BlockFooter*>(ptr);
	*footerPtr = BlockFooter();

#endif // DEBUG

	//printf("[FreeListAlloc][%p] Allocated %zu bytes.\n", this, size);
	return blockPtr;

};


void FreeListAlloc::Free(void* ptr)
{
	if (!ptr)
	{
		printf("[FreeListAlloc][%p] Delete was passed a null ptr!\n", this);
		assert(false && "Delete was passed a null ptr!");
		return;
	}

#ifdef _DEBUG
	
	assert(IsValidBlock(ptr) &&
		"Ptr was either already deleted or it had been written by an object bigger than its allocated size");

	InvalidateBlock(ptr);

#endif

	//Grab the BlockHeader pointer
	uint8_t* headerPtr = reinterpret_cast<uint8_t*>(ptr) - sizeof(BlockHeader);
	BlockHeader header = *reinterpret_cast<BlockHeader*>(headerPtr);

	headerPtr -= header.m_alignAdjustment;
	header.m_size += header.m_alignAdjustment;
	ptr = headerPtr;

	if (m_pFreeListRoot)
	{
		// Add new node to list
		FreeListNode* newNode = reinterpret_cast<FreeListNode*>(ptr);
		m_pFreeListHead->m_pNext = newNode;
		m_pFreeListHead = newNode;
		newNode->m_size = header.m_size;
		newNode->m_pNext = nullptr;

	}
	else
	{
		// else make this the new root
		m_pFreeListRoot = reinterpret_cast<FreeListNode*>(ptr);
		m_pFreeListRoot->m_size = header.m_size;
		m_pFreeListRoot->m_pNext = nullptr;
		m_pFreeListHead = m_pFreeListRoot;
	}

	//printf("[FreeListAlloc][%p] Freed %i bytes.\n", this, header.m_size);
	return;
}
