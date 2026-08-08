#include "pch.h"
#include "VKComputePass.h"

#include "Core/Platform/Rendering/Objects/Buffer.h"
#include "Core/Platform/Rendering/Objects/Mapper.h"
#include "Core/Platform/Rendering/Objects/Sampler.h"
#include "Core/Platform/Rendering/Objects/Shader.h"
#include "Core/Platform/Rendering/Objects/View.h"

#include "Core/Platform/Rendering/Vulkan/VKCommandEngine.h"
#include "Core/Platform/Rendering/Vulkan/VKPassBindings.h"
#include "Core/Platform/Rendering/Vulkan/VKRenderContext.h"

#include <cstdio>

VKComputePass::VKComputePass(PRenderContext* pContext) : ComputePass(pContext)
{
	m_pDevice = pContext->GetDevice();
}

VKComputePass::VKComputePass(PRenderContext* pContext, const Data& data) : ComputePass(pContext)
{
	m_pDevice = pContext->GetDevice();
	Init(data);
}

VKComputePass::~VKComputePass()
{
	if (m_pDevice == nullptr || m_pDevice->Get() == VK_NULL_HANDLE)
		return;

	vkDeviceWaitIdle(m_pDevice->Get());

	if (m_Pipeline != VK_NULL_HANDLE)
		vkDestroyPipeline(m_pDevice->Get(), m_Pipeline, nullptr);

	if (m_PipelineLayout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(m_pDevice->Get(), m_PipelineLayout, nullptr);

	m_DescriptorLayout.Destroy();
}

void VKComputePass::Init(const Data& data)
{
	m_Data = data;

	m_Bindings = VKBuildComputePassBindings(m_Data);

	if (!m_Bindings.empty())
	{
		VKApplyBindings(m_DescriptorLayout, m_Bindings);

		if (!m_DescriptorLayout.Build(m_pDevice, VKCommandEngine::m_uiFrameCount))
			fprintf(stderr, "[vulkan] '%s' descriptor layout failed\n", m_Data.m_Name.c_str());
	}

	if (!CreatePipeline())
		fprintf(stderr, "[vulkan] '%s' compute pipeline failed\n", m_Data.m_Name.c_str());
}

bool VKComputePass::CreatePipeline()
{
	if (m_Data.m_pShader == nullptr)
		return false;

	PShader* pShader = m_Data.m_pShader->GetNative();

	if (pShader == nullptr || !pShader->IsValid())
		return false;

	VkDescriptorSetLayout setLayout = m_DescriptorLayout.GetLayout();

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = setLayout != VK_NULL_HANDLE ? 1 : 0;
	layoutInfo.pSetLayouts = setLayout != VK_NULL_HANDLE ? &setLayout : nullptr;

	if (vkCreatePipelineLayout(m_pDevice->Get(), &layoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
		return false;

	VkPipelineShaderStageCreateInfo stage{};
	stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage.module = pShader->GetModule();
	stage.pName = "main";

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = stage;
	pipelineInfo.layout = m_PipelineLayout;

	if (vkCreateComputePipelines(m_pDevice->Get(), VK_NULL_HANDLE, 1,
	                             &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] vkCreateComputePipelines failed for '%s'\n", m_Data.m_Name.c_str());
		return false;
	}

	return true;
}

void VKComputePass::WriteDescriptors(VkDescriptorSet set)
{
	std::vector<VkWriteDescriptorSet> writes;
	std::vector<VkDescriptorBufferInfo> bufferInfos;
	std::vector<VkDescriptorImageInfo> imageInfos;
	std::vector<VkBufferView> texelViews;

	bufferInfos.reserve(m_Bindings.size());
	imageInfos.reserve(m_Bindings.size());
	texelViews.reserve(m_Bindings.size());

	for (const VKPassBinding& binding : m_Bindings)
	{
		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = set;
		write.dstBinding = binding.m_uiBinding;
		write.descriptorCount = 1;

		if (binding.m_Source == VKPassBinding::E_SOURCE_MAPPER)
		{
			Mapper* pMapper = const_cast<Mapper*>(static_cast<const Mapper*>(binding.m_pSource));

			if (pMapper == nullptr || pMapper->GetNative() == nullptr)
				continue;

			VkDescriptorBufferInfo info{};
			info.buffer = pMapper->GetNative()->GetBuffer();
			info.range = VK_WHOLE_SIZE;

			bufferInfos.push_back(info);

			write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write.pBufferInfo = &bufferInfos.back();
		}
		else if (binding.m_Source == VKPassBinding::E_SOURCE_BUFFER)
		{
			const Buffer* pBuffer = static_cast<const Buffer*>(binding.m_pSource);

			if (pBuffer == nullptr || pBuffer->GetNativeHandle() == 0)
				continue;

			VkDescriptorBufferInfo info{};
			info.buffer = reinterpret_cast<VkBuffer>(pBuffer->GetNativeHandle());
			info.offset = pBuffer->GetNativeOffset();
			info.range = pBuffer->GetTotalSize() > 0 ? pBuffer->GetTotalSize() : VK_WHOLE_SIZE;

			bufferInfos.push_back(info);

			write.descriptorType = binding.m_Kind == VKPassBinding::E_CONSTANT_BUFFER
				? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
				: VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write.pBufferInfo = &bufferInfos.back();
		}
		else if (binding.m_Source == VKPassBinding::E_SOURCE_VIEW)
		{
			View* pView = const_cast<View*>(static_cast<const View*>(binding.m_pSource));

			if (pView == nullptr || pView->GetNative() == nullptr)
				continue;

			VkDescriptorImageInfo info{};
			info.imageView = pView->GetNative()->GetOrCreateImageView(VK_IMAGE_VIEW_TYPE_2D);

			/* A compute pass writing an image needs GENERAL, not the
			   read-only layout a graphics pass would sample from. */
			const bool bIsStorage = binding.m_Kind == VKPassBinding::E_STORAGE_IMAGE;
			info.imageLayout = bIsStorage ? VK_IMAGE_LAYOUT_GENERAL
			                              : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			if (info.imageView == VK_NULL_HANDLE)
				continue;

			imageInfos.push_back(info);

			write.descriptorType = bIsStorage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
			                                  : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			write.pImageInfo = &imageInfos.back();
		}
		else if (binding.m_Source == VKPassBinding::E_SOURCE_SAMPLER)
		{
			Sampler* pSampler = const_cast<Sampler*>(static_cast<const Sampler*>(binding.m_pSource));

			if (pSampler == nullptr || !pSampler->IsInitialized())
				continue;

			VkDescriptorImageInfo info{};
			info.sampler = *static_cast<VkSampler*>(pSampler->GetNative());

			imageInfos.push_back(info);

			write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			write.pImageInfo = &imageInfos.back();
		}
		else
		{
			continue;
		}

		writes.push_back(write);
	}

	if (!writes.empty())
	{
		vkUpdateDescriptorSets(m_pDevice->Get(), static_cast<uint32_t>(writes.size()),
		                       writes.data(), 0, nullptr);
	}
}

void VKComputePass::Compute(PCommandEngine* pEngine)
{
	if (m_Pipeline == VK_NULL_HANDLE || pEngine == nullptr)
		return;

	/* Anything this pass writes has to be in GENERAL before the dispatch. */
	for (const VKPassBinding& binding : m_Bindings)
	{
		if (binding.m_Kind != VKPassBinding::E_STORAGE_IMAGE)
			continue;

		if (binding.m_Source == VKPassBinding::E_SOURCE_VIEW)
		{
			View* pView = const_cast<View*>(static_cast<const View*>(binding.m_pSource));

			if (pView != nullptr)
				pView->SetState(pEngine, E_STATE_COMMON_RESOURCE);
		}
	}

	pEngine->ApplyBarriers();

	VkCommandBuffer cmd = pEngine->GetCommandBuffer();

	const uint32_t uiTimestampBeginIndex = pEngine->WriteTimestampBegin();

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);

	if (m_DescriptorLayout.IsBuilt())
	{
		VkDescriptorSet set = m_DescriptorLayout.Allocate();

		if (set != VK_NULL_HANDLE)
		{
			WriteDescriptors(set);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout,
			                        0, 1, &set, 0, nullptr);
		}
	}

	vkCmdDispatch(cmd, m_Data.m_ThreadGroup.x, m_Data.m_ThreadGroup.y, m_Data.m_ThreadGroup.z);

	pEngine->WriteTimestampEnd(m_Data.m_Name, uiTimestampBeginIndex);
}
