#pragma once

#ifdef _ORBIS
#if defined(CUSTOM_ALLOCATOR)
#include "Core/Memory/Allocators/LinearAlloc.h"
#else
#include <Core/Memory/Allocators/ORBLinearAllocator.h>
#endif

#include <gnm/platform.h>
#include <gnm/RenderTarget.h>
#include <gnmx/gfxcontext.h>

#include "allocators.h"
#endif

#include "Core/Platform/Rendering/RenderDefines.h"

struct PlatformData
{
	PlatformData()
	{
#ifdef _ORBIS
		m_GpuMode = sce::Gnm::getGpuMode();

#ifdef CUSTOM_ALLOCATOR
		// Linear allocator
		m_pGarlicAllocator = new LinearAlloc();
		m_pOnionAllocator = new LinearAlloc();
#else
		m_pGarlicAllocator = new ORBLinearAllocator();
		m_pOnionAllocator = new ORBLinearAllocator();
#endif
#endif
	}

	~PlatformData()
	{
#ifdef _ORBIS
#ifndef CUSTOM_ALLOCATOR
		m_pGarlicAllocator->terminate();
		m_pOnionAllocator->terminate();
#endif

		delete m_pGarlicAllocator;
		delete m_pOnionAllocator;
#endif
	}

	PRenderPass* m_pScreenRenderPass = nullptr;

#ifdef _ORBIS
	typedef struct Context
	{
		sce::Gnmx::GfxContext    gfxContext;
		void                   *cueHeap;
		void                   *dcbBuffer;
		void                   *ccbBuffer;
		volatile uint32_t      *contextLabel;
	} ContextData;

	sce::Gnm::GpuMode m_GpuMode;

	int32_t m_WindowHandle = -1;
	static const uint32_t m_uiRenderContextCount = 2;

	uint32_t m_uiRenderContextIndex = 0;

	Context m_RenderContexts[m_uiRenderContextCount];
	Context* m_pRenderContext = &m_RenderContexts[0];

	sce::Gnmx::Toolkit::Allocators m_ToolkitAllocators;

#ifdef CUSTOM_ALLOCATOR
	// Linear allocator
	LinearAlloc* m_pGarlicAllocator = nullptr;
	LinearAlloc* m_pOnionAllocator = nullptr;
#else
	ORBLinearAllocator* m_pGarlicAllocator = nullptr;
	ORBLinearAllocator* m_pOnionAllocator = nullptr;
#endif
#endif
};