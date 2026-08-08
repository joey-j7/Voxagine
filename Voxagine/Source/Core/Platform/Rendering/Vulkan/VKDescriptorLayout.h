#pragma once

#include "Core/Platform/Rendering/Vulkan/VKShaderBindings.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

class VKDevice;

/* Descriptor set layouts and pools.
 *
 * This is the widest gap in the port. D3D12 root signatures let a pass declare
 * "a table of N SRVs at t0" and bind resources positionally at draw time.
 * Vulkan needs the exact set of bindings known up front, a pool sized for
 * them, and an allocated VkDescriptorSet whose contents are written before
 * use.
 *
 * Everything lands in set 0. The engine has no notion of update frequency to
 * split sets by, and inventing one here would be guessing ahead of the passes
 * that would have to use it. Binding numbers come from VKBindings, so they
 * match what DXC emitted for the SPIR-V.
 *
 * Bindless arrays (RenderPass::Data::m_uiBindlessResourceCount) use descriptor
 * indexing with a variable descriptor count, which is core since Vulkan 1.2. */
class VKDescriptorLayout
{
public:
	VKDescriptorLayout() = default;
	~VKDescriptorLayout();

	VKDescriptorLayout(const VKDescriptorLayout&) = delete;
	VKDescriptorLayout& operator=(const VKDescriptorLayout&) = delete;

	/* Declare bindings before Build(). uiRegister is the HLSL register number;
	   the VKBindings shift is applied here so callers never do it themselves. */
	void AddConstantBuffer(uint32_t uiRegister, VkShaderStageFlags stages);
	void AddTexture(uint32_t uiRegister, VkShaderStageFlags stages, uint32_t uiCount = 1);
	void AddStorageBuffer(uint32_t uiRegister, VkShaderStageFlags stages, uint32_t uiCount = 1);
	void AddStorageImage(uint32_t uiRegister, VkShaderStageFlags stages, uint32_t uiCount = 1);
	void AddSampler(uint32_t uiRegister, VkShaderStageFlags stages);

	/* A bindless array must be the last binding in the set - Vulkan only
	   permits VARIABLE_DESCRIPTOR_COUNT on the highest binding number. */
	void AddBindlessTextures(uint32_t uiRegister, VkShaderStageFlags stages, uint32_t uiMaxCount);

	/* uiMaxSets is how many sets the pool can hand out, normally one per frame
	   in flight. */
	bool Build(VKDevice* pDevice, uint32_t uiMaxSets);

	void Destroy();

	/* Returns VK_NULL_HANDLE when the pool is exhausted. */
	VkDescriptorSet Allocate();

	/* Recycles every set; only safe once the GPU is done with them. */
	void ResetPool();

	VkDescriptorSetLayout GetLayout() const { return m_Layout; }
	bool IsBuilt() const { return m_Layout != VK_NULL_HANDLE; }

	uint32_t GetBindingCount() const { return static_cast<uint32_t>(m_Bindings.size()); }

private:
	void Add(uint32_t uiBinding, VkDescriptorType type, VkShaderStageFlags stages, uint32_t uiCount);

	VKDevice* m_pDevice = nullptr;

	VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
	VkDescriptorPool m_Pool = VK_NULL_HANDLE;

	std::vector<VkDescriptorSetLayoutBinding> m_Bindings;

	bool m_bHasBindless = false;
	uint32_t m_uiBindlessMaxCount = 0;
};
