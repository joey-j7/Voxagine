#include "pch.h"

#include "Core/Platform/Rendering/Objects/Sampler.h"

#include "Core/Platform/Rendering/Vulkan/VKRenderContext.h"
#include "Core/Platform/Rendering/Vulkan/VKTranslate.h"

/* DX12 declared samplers as static samplers baked into the root signature, so
   this constructor had nothing to do. Vulkan needs a real VkSampler object. */

Sampler::Sampler(PRenderContext* pContext, const Info& info)
{
	m_pContext = pContext;
	m_Info = info;

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VKFilter(m_Info.m_FilterMode);
	samplerInfo.minFilter = VKFilter(m_Info.m_FilterMode);
	samplerInfo.mipmapMode = VKMipmapMode(m_Info.m_FilterMode);
	samplerInfo.addressModeU = VKAddressMode(m_Info.m_WrapMode);
	samplerInfo.addressModeV = VKAddressMode(m_Info.m_WrapMode);
	samplerInfo.addressModeW = VKAddressMode(m_Info.m_WrapMode);
	samplerInfo.mipLodBias = 0.f;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.f;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	VkSampler sampler = VK_NULL_HANDLE;

	if (vkCreateSampler(pContext->GetDevice()->Get(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] vkCreateSampler failed\n");
		return;
	}

	/* The interface stores this as void*. Held by pointer rather than by
	   punning the handle into one, so the destructor has something real to
	   delete - deleting a void* was undefined behaviour. */
	m_pNativeSampler = new VkSampler(sampler);
	m_bInitialized = true;
}

Sampler::~Sampler()
{
	if (m_pNativeSampler == nullptr)
		return;

	VkSampler* pSampler = static_cast<VkSampler*>(m_pNativeSampler);

	if (*pSampler != VK_NULL_HANDLE && m_pContext != nullptr)
		vkDestroySampler(m_pContext->GetDevice()->Get(), *pSampler, nullptr);

	delete pSampler;
	m_pNativeSampler = nullptr;
}
