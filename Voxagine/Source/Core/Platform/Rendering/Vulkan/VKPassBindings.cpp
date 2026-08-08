#include "VKPassBindings.h"

#include "VKRenderPass.h"

#include "VKDescriptorLayout.h"
#include "VKShaderBindings.h"

#include "Core/Platform/Rendering/Objects/Buffer.h"
#include "Core/Platform/Rendering/Objects/Mapper.h"
#include "Core/Platform/Rendering/Objects/Sampler.h"
#include "Core/Platform/Rendering/Objects/View.h"

#include <cstdio>

namespace
{
	/* Running per-class register counters, exactly as DXRenderPass::Init kept
	   uiCBVCount / uiSRVCount / uiUAVCount. */
	struct RegisterCounters
	{
		uint32_t m_uiConstant = 0;
		uint32_t m_uiTexture = 0;
		uint32_t m_uiUnordered = 0;
		uint32_t m_uiSampler = 0;
	};

	VKPassBinding MakeBinding(VKPassBinding::Kind kind, uint32_t uiRegister,
	                          VkShaderStageFlags stages, const void* pSource,
	                          const std::string& name, uint32_t uiCount = 1,
	                          VKPassBinding::Source source = VKPassBinding::E_SOURCE_NONE)
	{
		VKPassBinding binding;
		binding.m_Kind = kind;
		binding.m_Source = source;
		binding.m_uiRegister = uiRegister;
		binding.m_uiCount = uiCount;
		binding.m_Stages = stages;
		binding.m_pSource = pSource;
		binding.m_Name = name;

		switch (kind)
		{
		case VKPassBinding::E_CONSTANT_BUFFER:
			binding.m_uiBinding = VKBindings::ConstantBuffer(uiRegister);
			break;

		case VKPassBinding::E_STORAGE_BUFFER:
		case VKPassBinding::E_SAMPLED_IMAGE:
		case VKPassBinding::E_BINDLESS_TEXTURES:
		case VKPassBinding::E_UNIFORM_TEXEL_BUFFER:
			binding.m_uiBinding = VKBindings::Texture(uiRegister);
			break;

		case VKPassBinding::E_STORAGE_IMAGE:
		case VKPassBinding::E_STORAGE_TEXEL_BUFFER:
			binding.m_uiBinding = VKBindings::Unordered(uiRegister);
			break;

		case VKPassBinding::E_SAMPLER:
			binding.m_uiBinding = VKBindings::Sampler(uiRegister);
			break;
		}

		return binding;
	}

	/* E_STORAGE_BUFFER is the one kind that does not name its register class:
	   HLSL spells a read-only one StructuredBuffer (t) and a read-write one
	   RWStructuredBuffer (u), and Vulkan calls both a storage buffer. Anything
	   read-write has to go through here rather than MakeBinding, which can
	   only guess - and it guesses t, so a missed call silently lands the
	   descriptor on top of whichever texture holds that register. */
	VKPassBinding MakeUnorderedStorageBuffer(uint32_t uiRegister, VkShaderStageFlags stages,
	                                         const void* pSource, const std::string& name,
	                                         VKPassBinding::Source source)
	{
		VKPassBinding binding = MakeBinding(VKPassBinding::E_STORAGE_BUFFER, uiRegister,
		                                    stages, pSource, name, 1, source);
		binding.m_uiBinding = VKBindings::Unordered(uiRegister);

		return binding;
	}

	void AddBuffers(std::vector<VKPassBinding>& out, RegisterCounters& counters,
	                const std::vector<Buffer*>& buffers, VkShaderStageFlags stages)
	{
		for (Buffer* pBuffer : buffers)
		{
			if (pBuffer == nullptr)
				continue;

			const Buffer::Info& info = pBuffer->GetInfo();

			if (info.m_Type == Buffer::E_CONSTANT)
			{
				out.push_back(MakeBinding(VKPassBinding::E_CONSTANT_BUFFER,
				                          counters.m_uiConstant++, stages, pBuffer, info.m_Name, 1, VKPassBinding::E_SOURCE_BUFFER));
			}
			else if (info.m_GPUAccessType == E_READ_WRITE)
			{
				out.push_back(MakeUnorderedStorageBuffer(counters.m_uiUnordered++, stages, pBuffer,
				                                         info.m_Name, VKPassBinding::E_SOURCE_BUFFER));
			}
			else
			{
				out.push_back(MakeBinding(VKPassBinding::E_STORAGE_BUFFER,
				                          counters.m_uiTexture++, stages, pBuffer, info.m_Name, 1, VKPassBinding::E_SOURCE_BUFFER));
			}
		}
	}

	void AddMappers(std::vector<VKPassBinding>& out, RegisterCounters& counters,
	                const std::vector<Mapper*>& mappers, VkShaderStageFlags stages)
	{
		for (Mapper* pMapper : mappers)
		{
			if (pMapper == nullptr)
				continue;

			const Mapper::Info& info = pMapper->GetInfo();

			/* A mapper with a colour format is bound as an image; without one
			   it is a raw buffer. The DX12 path made the same split on
			   m_ColorFormat == E_UNKNOWN. */
			const bool bIsImage = info.m_ColorFormat != E_UNKNOWN;

			if (info.m_GPUAccessType == E_READ_WRITE)
			{
				if (bIsImage)
				{
					out.push_back(MakeBinding(VKPassBinding::E_STORAGE_TEXEL_BUFFER,
					                          counters.m_uiUnordered++, stages, pMapper, info.m_Name, 1, VKPassBinding::E_SOURCE_MAPPER));
				}
				else
				{
					out.push_back(MakeUnorderedStorageBuffer(counters.m_uiUnordered++, stages, pMapper,
					                                         info.m_Name, VKPassBinding::E_SOURCE_MAPPER));
				}
			}
			else if (bIsImage)
			{
				out.push_back(MakeBinding(VKPassBinding::E_UNIFORM_TEXEL_BUFFER,
				                          counters.m_uiTexture++, stages, pMapper, info.m_Name, 1, VKPassBinding::E_SOURCE_MAPPER));
			}
			else
			{
				out.push_back(MakeBinding(VKPassBinding::E_STORAGE_BUFFER,
				                          counters.m_uiTexture++, stages, pMapper, info.m_Name, 1, VKPassBinding::E_SOURCE_MAPPER));
			}
		}
	}

	void AddSamplers(std::vector<VKPassBinding>& out, RegisterCounters& counters,
	                 const std::vector<Sampler*>& samplers, VkShaderStageFlags stages)
	{
		for (Sampler* pSampler : samplers)
		{
			if (pSampler == nullptr)
				continue;

			out.push_back(MakeBinding(VKPassBinding::E_SAMPLER, counters.m_uiSampler++,
			                          stages, pSampler,
			                          "Sampler " + std::to_string(counters.m_uiSampler - 1), 1, VKPassBinding::E_SOURCE_SAMPLER));
		}
	}
}

std::vector<VKPassBinding> VKBuildRenderPassBindings(const RenderPass::Data& data)
{
	std::vector<VKPassBinding> bindings;
	RegisterCounters counters;

	/* DX12 gave buffers and mappers ALL visibility and textures PIXEL. Keeping
	   that split means a shader stage cannot read something the old backend
	   would not have let it read. */
	const VkShaderStageFlags allStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	const VkShaderStageFlags pixelStage = VK_SHADER_STAGE_FRAGMENT_BIT;

	AddBuffers(bindings, counters, data.m_Buffers, allStages);
	AddMappers(bindings, counters, data.m_Mappers, allStages);

	for (size_t i = 0; i < data.m_PassOutput.size(); ++i)
	{
		PRenderPass* pPass = data.m_PassOutput[i];

		if (pPass == nullptr)
			continue;

		for (uint32_t j = 0; j < pPass->GetData().m_uiRenderViewCount; ++j)
		{
			VKPassBinding binding = MakeBinding(VKPassBinding::E_SAMPLED_IMAGE,
			                                    counters.m_uiTexture++, pixelStage, pPass,
			                                    "Pass " + std::to_string(i) + " View " + std::to_string(j),
			                                    1, VKPassBinding::E_SOURCE_PASS);
			binding.m_uiViewIndex = j;

			bindings.push_back(binding);
		}
	}

	for (View* pTexture : data.m_Textures)
	{
		if (pTexture == nullptr)
			continue;

		bindings.push_back(MakeBinding(VKPassBinding::E_SAMPLED_IMAGE,
		                               counters.m_uiTexture++, pixelStage, pTexture,
		                               pTexture->GetInfo().m_Name, 1, VKPassBinding::E_SOURCE_VIEW));
	}

	/* Must be last: Vulkan only permits a variable descriptor count on the
	   highest binding in the set. DX12 had no such rule, but the bindless
	   array already came last there too.
	   m_uiBindlessResourceCount counts unbounded HLSL ranges, not resources -
	   the DX12 heap held 256 slots, so the variable count reserves that. */
	if (data.m_uiBindlessResourceCount > 0)
	{
		bindings.push_back(MakeBinding(VKPassBinding::E_BINDLESS_TEXTURES,
		                               counters.m_uiTexture, pixelStage, nullptr,
		                               "Unbounded Resources", VKPassBinding::m_uiBindlessCapacity));

		counters.m_uiTexture += data.m_uiBindlessResourceCount;
	}

	AddSamplers(bindings, counters, data.m_Samplers, pixelStage);

	return bindings;
}

std::vector<VKPassBinding> VKBuildComputePassBindings(const ComputePass::Data& data)
{
	std::vector<VKPassBinding> bindings;
	RegisterCounters counters;

	const VkShaderStageFlags stage = VK_SHADER_STAGE_COMPUTE_BIT;

	AddBuffers(bindings, counters, data.m_Buffers, stage);
	AddMappers(bindings, counters, data.m_Mappers, stage);

	for (View* pTexture : data.m_Textures)
	{
		if (pTexture == nullptr)
			continue;

		bindings.push_back(MakeBinding(VKPassBinding::E_SAMPLED_IMAGE,
		                               counters.m_uiTexture++, stage, pTexture,
		                               pTexture->GetInfo().m_Name, 1, VKPassBinding::E_SOURCE_VIEW));
	}

	if (data.m_uiBindlessResourceCount > 0)
	{
		bindings.push_back(MakeBinding(VKPassBinding::E_BINDLESS_TEXTURES,
		                               counters.m_uiTexture, stage, nullptr,
		                               "Unbounded Resources", VKPassBinding::m_uiBindlessCapacity));

		counters.m_uiTexture += data.m_uiBindlessResourceCount;
	}

	AddSamplers(bindings, counters, data.m_Samplers, stage);

	return bindings;
}

bool VKApplyBindings(VKDescriptorLayout& layout, const std::vector<VKPassBinding>& bindings)
{
	/* Bindings carry their final binding number and descriptor type already,
	   so nothing here re-derives either. Re-deriving the type from the shifted
	   binding number is exactly how a read-write storage buffer ends up
	   declared as a storage image. */
	for (const VKPassBinding& binding : bindings)
	{
		if (binding.m_Kind == VKPassBinding::E_BINDLESS_TEXTURES)
		{
			layout.AddExplicitBindless(binding.m_uiBinding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			                           binding.m_Stages, binding.m_uiCount);
			continue;
		}

		VkDescriptorType type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

		switch (binding.m_Kind)
		{
		case VKPassBinding::E_CONSTANT_BUFFER: type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
		case VKPassBinding::E_UNIFORM_TEXEL_BUFFER: type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER; break;
		case VKPassBinding::E_STORAGE_TEXEL_BUFFER: type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER; break;
		case VKPassBinding::E_STORAGE_BUFFER:  type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
		case VKPassBinding::E_SAMPLED_IMAGE:   type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;  break;
		case VKPassBinding::E_STORAGE_IMAGE:   type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;  break;
		case VKPassBinding::E_SAMPLER:         type = VK_DESCRIPTOR_TYPE_SAMPLER;        break;
		case VKPassBinding::E_BINDLESS_TEXTURES: break;
		}

		layout.AddExplicit(binding.m_uiBinding, type, binding.m_Stages, binding.m_uiCount);
	}

	return true;
}
