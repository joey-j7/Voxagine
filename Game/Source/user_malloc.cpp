#include "pch.h"

#ifdef _ORBIS

/* SIE CONFIDENTIAL
 PlayStation(R)4 Programmer Tool Runtime Library Release 06.008.001
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

//#include "ASanUtils.h"

#include <algorithm>
#include <assert.h>
#include <errno.h>
#include <kernel.h>
#include <mspace.h>
#include <sceerror.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <mat.h>

#include <sanitizer/asan_interface.h>


#define HEAP_SIZE (512u * 1024u * 1024u)

static SceLibcMspace s_mspace;
static off_t s_memStart;
static size_t s_memLength = HEAP_SIZE;
static size_t s_memAlign = 2 * 1024 * 1024;

//E Mutex to synchronize shadow memory poisoning when running in ASan mode.
//J ASanモード実行時にシャドウメモリ有害化を同期するミューテックス
#ifdef ENABLE_ASAN
static ScePthreadMutex s_mtx;

#define INIT_ASAN_MUTEX() \
	do { \
		if (SCE_OK != scePthreadMutexInit(&s_mtx, nullptr, "AllocatorMutex")) \
			abort(); \
	} while(0)

#define DESTROY_ASAN_MUTEX() \
	do { \
		if (SCE_OK != scePthreadMutexDestroy(&s_mtx)) \
			abort(); \
	} while(0)

#define LOCK_ASAN_MUTEX() \
	do { \
		if (SCE_OK != scePthreadMutexLock(&s_mtx)) \
			abort(); \
	} while(0)

#define UNLOCK_ASAN_MUTEX() \
	do { \
		if (SCE_OK != scePthreadMutexUnlock(&s_mtx)) \
			abort(); \
	} while(0)

#endif /** ENABLE_ASAN **/

extern "C" {
int user_malloc_init(void);
int user_malloc_finalize(void);
void *user_malloc(size_t size);
void user_free(void *ptr);
void *user_calloc(size_t nelem, size_t size);
void *user_realloc(void *ptr, size_t size);
void *user_memalign(size_t boundary, size_t size);
int user_posix_memalign(void **ptr, size_t boundary, size_t size);
void *user_reallocalign(void *ptr, size_t size, size_t boundary);
int user_malloc_stats(SceLibcMallocManagedSize *mmsize);
int user_malloc_stats_fast(SceLibcMallocManagedSize *mmsize);
size_t user_malloc_usable_size(void *ptr);
}

//E Replace _malloc_init function.
//J _malloc_init 関数と置き換わる
int user_malloc_init(void)
{
	//E No need to lock the mutex under ASan mode since this function isn't executed in parallel with other user_* functions
	//J この関数は他のuser_*関数と並列に実行されることはないので、ASanモードでもミューテックスをロックする必要は無い
	int res;
	void *addr;

#ifdef ENABLE_ASAN
	INIT_ASAN_MUTEX();
#endif
	
	//E Allocate direct memory
	//J ダイレクトメモリを割り当てる
	res = sceKernelAllocateDirectMemory(0, SCE_KERNEL_MAIN_DMEM_SIZE, s_memLength, s_memAlign, SCE_KERNEL_WB_ONION, &s_memStart);
	if (res < 0) {
		//E Error handling
		//J エラー処理
		return 1;
	}

	addr = NULL;
	//E Map direct memory to the process address space
	//J ダイレクトメモリをプロセスアドレス空間にマップする
	res = sceKernelMapDirectMemory(&addr, HEAP_SIZE, SCE_KERNEL_PROT_CPU_READ | SCE_KERNEL_PROT_CPU_WRITE, 0, s_memStart, s_memAlign);
	if (res < 0) {
		//E Error handling
		//J エラー処理
		return 1;
	}

	//E Generate mspace
	//J mspace を生成する
	s_mspace = sceLibcMspaceCreate("User Malloc", addr, HEAP_SIZE, 0);
	if (s_mspace == NULL) {
		//E Error handling
		//J エラー処理
		return 1;
	}

	// E Poison the memory pool for AddressSanitizer (no-op if it's disabled)
	// J AddressSanitizerのためにメモリプールを有害化する（AddressSanitizerが無効の場合、何もしない）
	ASAN_POISON_MEMORY_REGION(s_mspace, HEAP_SIZE);

	return 0;
}

//E Replace _malloc_finalize function.
//J _malloc_finalize 関数と置き換わる
int user_malloc_finalize(void)
{
	int res;

	// E Unpoison the memory pool for AddressSanitizer (no-op if it's disabled)
	// J AddressSanitizerのためにメモリプールを無害化する（AddressSanitizerが無効の場合、何もしない）
	ASAN_UNPOISON_MEMORY_REGION(s_mspace, HEAP_SIZE);

	if (s_mspace != NULL) {
		//E Free mspace
		//J mspace を解放する
		sceLibcMspaceDestroy(s_mspace);
	}

	//E Release direct memory
	//J ダイレクトメモリを解放する
	res = sceKernelReleaseDirectMemory(s_memStart, s_memLength);
	if (res < 0) {
		//E Error handling
		//J エラー処理
#ifdef ENABLE_ASAN
		DESTROY_ASAN_MUTEX();
#endif		
		return 1;
	}

#ifdef ENABLE_ASAN
	DESTROY_ASAN_MUTEX();
#endif

	return 0;
}

//E Replace malloc function.
//J malloc 関数と置き換わる
void *user_malloc(size_t size)
{
#ifndef ENABLE_ASAN

	void* ptr = sceLibcMspaceMalloc(s_mspace, size);
	sceMatAlloc(ptr, size, 0, SCEMAT_GROUP_AUTO);
	return ptr;
#else

	if (SIZE_MAX - ALLOC_OFFSET < size)
		return nullptr;

	LOCK_ASAN_MUTEX();

	void *orig_ret = sceLibcMspaceMalloc(s_mspace, size + ALLOC_OFFSET);
	if (!orig_ret) {
		UNLOCK_ASAN_MUTEX();
		return nullptr;
	}

	void *ret = (char*)orig_ret + ALLOC_OFFSET;
	ASAN_UNPOISON_MEMORY_REGION(orig_ret, size + ALLOC_OFFSET);
	{
		MetadataView view(ret);
		Metadata &m = view.getMetadata();
		m.m_size = size;
		m.m_originalPtr = orig_ret;
	}
	UNLOCK_ASAN_MUTEX();

	return ret;
#endif
}

//E Replace free function.
//J free 関数と置き換わる
void user_free(void *ptr)
{
#ifdef ENABLE_ASAN
	if (!ptr)
		return;

	LOCK_ASAN_MUTEX();
	{
		MetadataView view(ptr);
		Metadata &m = view.getMetadata();
		ptr = m.m_originalPtr;
		size_t size = sceLibcMspaceMallocUsableSize(ptr);
		assert(m.m_size <= size);
		//E We might poison more memory, since UsableSize might be bigger than the requested size
		//J UsableSizeが要求サイズより大きいことがあるので、より大きいメモリを有害化するかもしれない
		ASAN_POISON_MEMORY_REGION(ptr, size);
	}
#endif
	sceMatFree(ptr);
	sceLibcMspaceFree(s_mspace, ptr);

#ifdef ENABLE_ASAN
	UNLOCK_ASAN_MUTEX();
#endif

}

//E Replace calloc function.
//J calloc 関数と置き換わる
void *user_calloc(size_t nelem, size_t size)
{
#ifndef ENABLE_ASAN
	void* ptr = sceLibcMspaceCalloc(s_mspace, nelem, size);
	sceMatAlloc(ptr, size * nelem, 0, SCEMAT_GROUP_AUTO);
	return ptr;
#else
	size_t adjusted_size;
	size_t adjusted_nelem;
	if (size != 0) {
		adjusted_size = size;
		adjusted_nelem = nelem + (ALLOC_OFFSET - 1) / size + 1;
	} else {
		//E If size is zero, allocate only enough for the metadata.
		//J sizeがゼロの場合、メタデータに十分なだけ割り付ける
		adjusted_size = ALLOC_OFFSET;
		adjusted_nelem = 1;
	}

	LOCK_ASAN_MUTEX();

	void *orig_ret = sceLibcMspaceCalloc(s_mspace, adjusted_nelem, adjusted_size);
	if (!orig_ret) {
		UNLOCK_ASAN_MUTEX();
		return nullptr;
	}

	void *ret = (char*)orig_ret + ALLOC_OFFSET;
	ASAN_UNPOISON_MEMORY_REGION(orig_ret, adjusted_nelem * adjusted_size);
	{
		MetadataView view(ret);
		Metadata &m = view.getMetadata();
		m.m_size = size * nelem;
		m.m_originalPtr = orig_ret;
	}
	UNLOCK_ASAN_MUTEX();

	return ret;
#endif
}

//E Replace realloc function.
//J realloc 関数と置き換わる
void *user_realloc(void *ptr, size_t size)
{
#ifndef ENABLE_ASAN
	sceMatReallocBegin(ptr, size, SCEMAT_GROUP_AUTO);
	void* newPtr = sceLibcMspaceRealloc(s_mspace, ptr, size);
	sceMatReallocEnd(newPtr, size, 0);
	return newPtr;
#else
	if (SIZE_MAX - ALLOC_OFFSET < size)
		return nullptr;

	LOCK_ASAN_MUTEX();

	void *orig_ret = sceLibcMspaceMalloc(s_mspace, size + ALLOC_OFFSET);
	if (!orig_ret) {
		UNLOCK_ASAN_MUTEX();
		return nullptr;
	}

	void *ret = (char*)orig_ret + ALLOC_OFFSET;
	ASAN_UNPOISON_MEMORY_REGION(orig_ret, size + ALLOC_OFFSET);
	{
		MetadataView view(ret);
		Metadata &m = view.getMetadata();
		m.m_size = size;
		m.m_originalPtr = orig_ret;
	}
	if (ptr) {
		MetadataView oldView(ptr);
		Metadata &oldM = oldView.getMetadata();

		memcpy(ret, ptr, oldM.m_size > size ? size : oldM.m_size);

		ptr = oldM.m_originalPtr;
		size_t oldSize = sceLibcMspaceMallocUsableSize(ptr);
		assert(oldM.m_size <= oldSize);
		ASAN_POISON_MEMORY_REGION(ptr, oldSize);
		sceLibcMspaceFree(s_mspace, ptr);
	}

	UNLOCK_ASAN_MUTEX();

	return ret;
#endif
}

//E Replace memalign function.
//J memalign 関数と置き換わる
void *user_memalign(size_t boundary, size_t size)
{
#ifndef ENABLE_ASAN
	void* ptr = sceLibcMspaceMemalign(s_mspace, boundary, size);
	sceMatAlloc(ptr, size, 0, SCEMAT_GROUP_AUTO);
	return ptr;
#else
	size_t offset = std::max(boundary, ALLOC_OFFSET);

	if (SIZE_MAX - offset < size)
		return nullptr;

	LOCK_ASAN_MUTEX();

	void *orig_ret = sceLibcMspaceMemalign(s_mspace, boundary, size + offset);
	if (!orig_ret) {
		UNLOCK_ASAN_MUTEX();
		return nullptr;
	}
	void *ret = (char*)orig_ret + offset;
	ASAN_UNPOISON_MEMORY_REGION(orig_ret, size + offset);
	{
		MetadataView view(ret);
		Metadata &m = view.getMetadata();
		m.m_size = size;
		m.m_originalPtr = orig_ret;
	}
	UNLOCK_ASAN_MUTEX();

	return ret;
#endif
}

//E Replace posix_memalign function.
//J posix_memalign 関数と置き換わる
int user_posix_memalign(void **ptr, size_t boundary, size_t size)
{
#ifndef ENABLE_ASAN
	int ret = sceLibcMspacePosixMemalign(s_mspace, ptr, boundary, size);
	sceMatAlloc(*ptr, size, 0, SCEMAT_GROUP_AUTO);
	return ret;
#else
	size_t offset = std::max(boundary, ALLOC_OFFSET);

	if (SIZE_MAX - offset < size)
		return ENOMEM;

	LOCK_ASAN_MUTEX();

	void *orig_ptr;
	int ret = sceLibcMspacePosixMemalign(s_mspace, &orig_ptr, boundary, size + offset);
	if (ret != SCE_OK) {
		UNLOCK_ASAN_MUTEX();
		return ret;
	}

	*ptr = (char*)orig_ptr + offset;
	ASAN_UNPOISON_MEMORY_REGION(orig_ptr, size + offset);
	{
		MetadataView view(*ptr);
		Metadata &m = view.getMetadata();
		m.m_size = size;
		m.m_originalPtr = orig_ptr;
	}
	UNLOCK_ASAN_MUTEX();

	return ret;
#endif
}

//E Replace reallocalign function.
//J reallocalign 関数と置き換わる
void *user_reallocalign(void *ptr, size_t size, size_t boundary)
{
#ifndef ENABLE_ASAN
	sceMatReallocBegin(ptr, size, SCEMAT_GROUP_AUTO);
	void* alloc = sceLibcMspaceReallocalign(s_mspace, ptr, boundary, size);
	sceMatReallocEnd(alloc, size, 0);

	return alloc;
#else
	size_t offset = std::max(boundary, ALLOC_OFFSET);

	if (SIZE_MAX - offset < size)
		return nullptr;

	LOCK_ASAN_MUTEX();

	void *orig_ret = sceLibcMspaceMemalign(s_mspace, boundary, size + offset);
	if (!orig_ret) {
		UNLOCK_ASAN_MUTEX();
		return nullptr;
	}

	void *ret = (char*)orig_ret + ALLOC_OFFSET;
	ASAN_UNPOISON_MEMORY_REGION(orig_ret, size + ALLOC_OFFSET);
	{
		MetadataView view(ret);
		Metadata &m = view.getMetadata();
		m.m_size = size;
		m.m_originalPtr = orig_ret;
	}
	if (ptr) {
		MetadataView oldView(ptr);
		Metadata &oldM = oldView.getMetadata();

		memcpy(ret, ptr, oldM.m_size > size ? size : oldM.m_size);

		ptr = oldM.m_originalPtr;
		size_t oldSize = sceLibcMspaceMallocUsableSize(ptr);
		assert(oldM.m_size <= oldSize);
		ASAN_POISON_MEMORY_REGION(ptr, oldSize);
		sceLibcMspaceFree(s_mspace, ptr);
	}

	UNLOCK_ASAN_MUTEX();

	return ret;
#endif
}

//E Replace malloc_stats function.
//J malloc_stats 関数と置き換わる
int user_malloc_stats(SceLibcMallocManagedSize *mmsize)
{
	return sceLibcMspaceMallocStats(s_mspace, mmsize);
}

//E Replace malloc_stats_fast function.
//J malloc_stata_fast 関数と置き換わる
int user_malloc_stats_fast(SceLibcMallocManagedSize *mmsize)
{
	return sceLibcMspaceMallocStatsFast(s_mspace, mmsize);
}

//E Replace malloc_usable_size function.
//J malloc_usable_size 関数と置き換わる
size_t user_malloc_usable_size(void *ptr)
{
#ifndef ENABLE_ASAN
	return sceLibcMspaceMallocUsableSize(ptr);
#else
	size_t size;

	if (!ptr)
		return 0;

	LOCK_ASAN_MUTEX();
	{
		MetadataView view(ptr);
		Metadata &m = view.getMetadata();
		size = m.m_size;
	}
	UNLOCK_ASAN_MUTEX();

	return size;
#endif
}

#endif