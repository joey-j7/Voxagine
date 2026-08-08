#pragma once

#include "Core/Platform/Rendering/RenderContext.h"

#include "Core/Platform/Rendering/Vulkan/VKAllocator.h"
#include "Core/Platform/Rendering/Vulkan/VKDevice.h"
#include "Core/Platform/Rendering/Vulkan/VKSwapchain.h"

/* Vulkan implementation of the engine's RenderContext.
 *
 * Owns the device, allocator and swapchain that everything else borrows.
 * GetDevice() and GetAllocator() are what View, Mapper, Shader and the passes
 * reach for, the same way they used DX12RenderContext::GetDevice(). */
class VKRenderContext : public RenderContext
{
public:
	VKRenderContext(Platform* pPlatform);
	virtual ~VKRenderContext();

	virtual void Initialize() override;
	virtual void Deinitialize() override;

	virtual void Clear() override;
	virtual bool Present() override;

	virtual bool OnResize(uint32_t uiWidth, uint32_t uiHeight) override;

	VKDevice* GetDevice() { return &m_Device; }
	const VKAllocator* GetAllocator() const { return &m_Allocator; }

	VKSwapchain* GetSwapchain() { return &m_Swapchain; }

	VkSurfaceKHR GetSurface() const { return m_Surface; }

private:
	virtual void LoadShader(ShaderReference* pShaderReference) override;
	virtual void DestroyShader(const ShaderReference* pShaderReference) override;

	/* Brings up instance, surface, device and swapchain. Separate from
	   Initialize so a failure can be reported before the base class starts
	   building passes against a device that does not exist. */
	bool InitializeBackend();

	VKDevice m_Device;
	VKAllocator m_Allocator;
	VKSwapchain m_Swapchain;

	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

	bool m_bBackendReady = false;
};
