#include "pch.h"

#include "BaseAlloc.h"
#include <stdio.h>


void* BaseAlloc::AllocMem(const size_t sizeInBytes, const MemoryType type, const int memoryProtection)
{
#if _ORBIS

	int32_t ret;

	m_memLength = sizeInBytes;

	assert( m_memLength % (16 * 1024) == 0 && "Size of memory to allocate in bytes, needs to be larger than 0 and a multiple of 16 KiB");
	static_assert( s_memAlign % (16 * 1024) == 0, "Alignment of memory to allocate in bytes, needs to be 0 or a multiple of 16 KiB and a power of 2)");

	ret = sceKernelAllocateDirectMemory(
		0,
		SCE_KERNEL_MAIN_DMEM_SIZE,
		m_memLength,
		s_memAlign,
		type,
		&m_dmemOffset);

	if (ret != SCE_OK)
	{
		printf("sceKernelAllocateDirectMemory failed: 0x%08X\n", ret);
		return nullptr;
	}

	void *baseAddress = NULL;
	ret = sceKernelMapDirectMemory(
		&baseAddress,
		m_memLength,
		memoryProtection,
		0,
		m_dmemOffset,
		s_memAlign);

	if (ret != SCE_OK)
	{
		printf("sceKernelMapDirectMemory failed: 0x%08X\n", ret);
		return nullptr;
	}

	m_memBaseAddr = static_cast<uint8_t*>(baseAddress);

	return m_memBaseAddr;

#else
	return malloc(sizeInBytes);
#endif
}

void BaseAlloc::FreeMem(void* ptr)
{
#if _ORBIS

	int tmpRet, ret = SCE_OK;

	if (m_memBaseAddr)
	{
		tmpRet = sceKernelMunmap(m_memBaseAddr, m_memLength);
		if (tmpRet != SCE_OK)
		{
			printf("sceKernelMunmap failed: 0x%08X\n", tmpRet);
			ret = tmpRet;
		}
	}

	if (m_dmemOffset)
	{
		tmpRet = sceKernelReleaseDirectMemory(m_dmemOffset, m_memLength);
		if (tmpRet != SCE_OK)
		{
			printf("sceKernelReleaseDirectMemory failed: 0x%08X\n", tmpRet);
			ret = tmpRet;
		}
	}

	m_memBaseAddr = NULL;
	m_dmemOffset = 0;

#else
	return free(ptr);
#endif
}
