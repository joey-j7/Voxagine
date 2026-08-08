#include "VKDescriptorLayout.h"

#include "VKDevice.h"

#include <cstdio>
#include <unordered_map>

VKDescriptorLayout::~VKDescriptorLayout()
{
	Destroy();
}

void VKDescriptorLayout::Add(uint32_t uiBinding, VkDescriptorType type,
                             VkShaderStageFlags stages, uint32_t uiCount)
{
	VkDescriptorSetLayoutBinding binding{};
	binding.binding = uiBinding;
	binding.descriptorType = type;
	binding.descriptorCount = uiCount;
	binding.stageFlags = stages;

	m_Bindings.push_back(binding);
}

void VKDescriptorLayout::AddConstantBuffer(uint32_t uiRegister, VkShaderStageFlags stages)
{
	Add(VKBindings::ConstantBuffer(uiRegister), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stages, 1);
}

void VKDescriptorLayout::AddTexture(uint32_t uiRegister, VkShaderStageFlags stages, uint32_t uiCount)
{
	Add(VKBindings::Texture(uiRegister), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, stages, uiCount);
}

void VKDescriptorLayout::AddStorageBuffer(uint32_t uiRegister, VkShaderStageFlags stages, uint32_t uiCount)
{
	/* StructuredBuffer/RWStructuredBuffer both land here; HLSL's t and u
	   registers are already separated by the binding shift. */
	Add(VKBindings::Texture(uiRegister), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stages, uiCount);
}

void VKDescriptorLayout::AddStorageImage(uint32_t uiRegister, VkShaderStageFlags stages, uint32_t uiCount)
{
	Add(VKBindings::Unordered(uiRegister), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, stages, uiCount);
}

void VKDescriptorLayout::AddSampler(uint32_t uiRegister, VkShaderStageFlags stages)
{
	Add(VKBindings::Sampler(uiRegister), VK_DESCRIPTOR_TYPE_SAMPLER, stages, 1);
}

void VKDescriptorLayout::AddBindlessTextures(uint32_t uiRegister, VkShaderStageFlags stages,
                                             uint32_t uiMaxCount)
{
	Add(VKBindings::Texture(uiRegister), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, stages, uiMaxCount);

	m_bHasBindless = true;
	m_uiBindlessBinding = VKBindings::Texture(uiRegister);
	m_uiBindlessMaxCount = uiMaxCount;
}

void VKDescriptorLayout::AddExplicit(uint32_t uiBinding, VkDescriptorType type,
                                     VkShaderStageFlags stages, uint32_t uiCount)
{
	Add(uiBinding, type, stages, uiCount);
}

void VKDescriptorLayout::AddExplicitBindless(uint32_t uiBinding, VkDescriptorType type,
                                             VkShaderStageFlags stages, uint32_t uiMaxCount)
{
	Add(uiBinding, type, stages, uiMaxCount);

	m_bHasBindless = true;
	m_uiBindlessBinding = uiBinding;
	m_uiBindlessMaxCount = uiMaxCount;
}

bool VKDescriptorLayout::Build(VKDevice* pDevice, uint32_t uiMaxSets)
{
	Destroy();

	m_pDevice = pDevice;

	if (m_Bindings.empty())
	{
		fprintf(stderr, "[vulkan] descriptor layout built with no bindings\n");
		return false;
	}

	std::vector<VkDescriptorBindingFlags> flags(m_Bindings.size(), 0);

	if (m_bHasBindless)
	{
		/* The array is allocated at its full fixed capacity rather than with
		   VARIABLE_DESCRIPTOR_COUNT: Vulkan only allows a variable count on
		   the set's highest binding number, and the register shifts place the
		   t-class bindless array below any u or s binding. A few hundred
		   fixed descriptors cost nothing. */
		for (size_t i = 0; i < m_Bindings.size(); ++i)
		{
			if (m_Bindings[i].binding == m_uiBindlessBinding)
			{
				flags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
				           VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
			}
		}
	}

	VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
	flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	flagsInfo.bindingCount = static_cast<uint32_t>(flags.size());
	flagsInfo.pBindingFlags = flags.data();

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(m_Bindings.size());
	layoutInfo.pBindings = m_Bindings.data();

	if (m_bHasBindless)
	{
		layoutInfo.pNext = &flagsInfo;
		layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	}

	if (vkCreateDescriptorSetLayout(pDevice->Get(), &layoutInfo, nullptr, &m_Layout) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] vkCreateDescriptorSetLayout failed\n");
		return false;
	}

	/* Pool sizes are the per-set counts multiplied by the number of sets. */
	std::unordered_map<int, uint32_t> counts;

	for (const VkDescriptorSetLayoutBinding& binding : m_Bindings)
		counts[static_cast<int>(binding.descriptorType)] += binding.descriptorCount * uiMaxSets;

	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.reserve(counts.size());

	for (const std::pair<const int, uint32_t>& entry : counts)
	{
		VkDescriptorPoolSize size{};
		size.type = static_cast<VkDescriptorType>(entry.first);
		size.descriptorCount = entry.second;

		poolSizes.push_back(size);
	}

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.maxSets = uiMaxSets;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();

	if (m_bHasBindless)
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

	if (vkCreateDescriptorPool(pDevice->Get(), &poolInfo, nullptr, &m_Pool) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] vkCreateDescriptorPool failed\n");

		vkDestroyDescriptorSetLayout(pDevice->Get(), m_Layout, nullptr);
		m_Layout = VK_NULL_HANDLE;

		return false;
	}

	return true;
}

VkDescriptorSet VKDescriptorLayout::Allocate()
{
	if (m_Layout == VK_NULL_HANDLE)
		return VK_NULL_HANDLE;

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_Pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_Layout;

	VkDescriptorSet set = VK_NULL_HANDLE;

	const VkResult result = vkAllocateDescriptorSets(m_pDevice->Get(), &allocInfo, &set);

	if (result != VK_SUCCESS)
	{
		/* OUT_OF_POOL_MEMORY means maxSets was set too low for how many the
		   caller actually wants. */
		fprintf(stderr, "[vulkan] vkAllocateDescriptorSets failed (%d)\n", result);
		return VK_NULL_HANDLE;
	}

	return set;
}

void VKDescriptorLayout::ResetPool()
{
	if (m_Pool != VK_NULL_HANDLE)
		vkResetDescriptorPool(m_pDevice->Get(), m_Pool, 0);
}

void VKDescriptorLayout::Destroy()
{
	if (m_pDevice == nullptr)
		return;

	if (m_Pool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_pDevice->Get(), m_Pool, nullptr);
		m_Pool = VK_NULL_HANDLE;
	}

	if (m_Layout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(m_pDevice->Get(), m_Layout, nullptr);
		m_Layout = VK_NULL_HANDLE;
	}

	m_pDevice = nullptr;
}
