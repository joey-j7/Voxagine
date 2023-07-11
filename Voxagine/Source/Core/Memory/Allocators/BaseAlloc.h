#pragma once

#ifndef _ORBIS
#include <malloc.h>
#define SCE_KERNEL_WB_ONION 0 
#define SCE_KERNEL_WB_GARLIC 1
#define SCE_KERNEL_PROT_CPU_RW 0
#define SCE_OK 0
#else
#include <kernel.h>
#include <mspace.h>
#include <scebase.h>
#include <gnm.h>
#endif

#include <exception>
#include <stdint.h>

enum MemoryType
{
	ONION = SCE_KERNEL_WB_ONION,
	GARLIC = SCE_KERNEL_WB_GARLIC
};

class BaseAlloc
{
public:

	BaseAlloc() = default;

	//Required for 64-bit physical addresses
	using off_t = int64_t;

	virtual int Initialize(
		const size_t bufferSizeInBytes, 
		const MemoryType type = ONION, 
		const int memoryProtection = SCE_KERNEL_PROT_CPU_RW) = 0;

	virtual int Initialize(BaseAlloc* parent, const size_t bufferSizeInBytes) = 0;

	virtual int Destroy() = 0;

	virtual void* Allocate(size_t size, size_t align) = 0;
	//virtual void Free( void* ptr, size_t size ) = 0; Future optimization
	virtual void Free( void* ptr ) = 0;

protected:

	void* AllocMem(
		const size_t SizeInBytes,
		const MemoryType type,
		const int memoryProtection);

	void FreeMem(void* ptr);

	BaseAlloc* m_pParent = nullptr;

public:

	struct BlockHeader
	{
		BlockHeader(size_t size, size_t adjustment) :
			m_size(size),
			m_alignAdjustment(adjustment)
		{
		}

		size_t m_size = 0;
		size_t m_alignAdjustment = 0;
	};



	struct BlockFooter
	{
		int32_t m_guard = 42;
	};


	virtual bool HasBaseAddr() = 0;

#ifdef _DEBUG

public:

	template< typename T = BlockHeader, typename U = BlockFooter>
	bool IsValidBlock(void* ptr)
	{
		assert(ptr && "ptr was null!");

		uint8_t* blockPtr = static_cast<uint8_t*>(ptr);
		T* headerPtr = reinterpret_cast<T*>(blockPtr - sizeof(T));
		U* footerPtr = reinterpret_cast<U*>(blockPtr + headerPtr->m_size );

		return footerPtr->m_guard == 42;
	}

	template< typename T = BlockHeader, typename U = BlockFooter>
	void InvalidateBlock(void* ptr)
	{
		assert(ptr && "ptr was null!");

		uint8_t* blockPtr = static_cast<uint8_t*>(ptr);
		T* headerPtr = reinterpret_cast<T*>(blockPtr - sizeof(T));
		U* footerPtr = reinterpret_cast<U*>(blockPtr + headerPtr->m_size + headerPtr->m_alignAdjustment);

		footerPtr->m_guard = -1;
	}

#endif // _DEBUG

private:

#ifdef _ORBIS
	uint8_t* m_memBaseAddr;
	off_t m_dmemOffset;
	size_t m_memLength;
	static const size_t s_memAlign = 128 * 1024 * 1024;

	static void* AllocCallback(void* instance, uint32_t size, sce::Gnm::AlignmentType alignment)
	{
		BaseAlloc *allocator = static_cast<BaseAlloc*>(instance);
		return allocator->Allocate(size, alignment);
	}

	static void FreeCallback(void* instance, void* ptr)
	{
		BaseAlloc *allocator = static_cast<BaseAlloc*>(instance);
		allocator->Free(ptr);
	}

#endif // _ORBIS	

};


template<typename T, typename U> inline T* alignPtrTo(const T* ptr, const U alignment)
{
	return reinterpret_cast<T*>( ((reinterpret_cast<U>(ptr) + alignment - T(1)) / alignment) * alignment );
}

inline size_t alignAdjustment(const uint8_t* address, size_t alignment)
{
	size_t adjustment = alignment - (reinterpret_cast<const uintptr_t>(address) & static_cast<uintptr_t>(alignment - 1));


	if (adjustment == alignment) 
		return 0; // Already aligned

	return adjustment;
}