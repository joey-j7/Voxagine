#include "pch.h"

#include "Core/Platform/Rendering/Objects/View.h"

#include "Core/Platform/Rendering/Vulkan/VKRenderContext.h"
#include "Core/Platform/Rendering/Vulkan/VKTranslate.h"

/* View owns a VKResource image plus the VkImageView onto it.
 *
 * The DX12 implementation also wrote a descriptor into a heap slot for every
 * target it was attached to (CreateTarget). Vulkan writes descriptors into a
 * pass's descriptor set at bind time instead, so that work moves to
 * VKRenderPass and CreateTarget only has to record the association. */

View::~View()
{
	if (m_pNativeTexture != nullptr)
	{
		m_pNativeTexture->Destroy();
		delete m_pNativeTexture;
		m_pNativeTexture = nullptr;
	}

	delete[] m_pData;
	m_pData = nullptr;
}

PResourceStates View::SetState(PCommandEngine* pEngine, PResourceStates state)
{
	const PResourceStates oldState = m_Info.m_State;

	/* DX12 only bookkept here and left the caller to push a barrier. Queuing
	   it means a state change can never be recorded without the matching
	   transition being emitted. */
	if (pEngine != nullptr && m_pNativeTexture != nullptr)
		pEngine->QueueBarrier(m_pNativeTexture, state);

	m_Info.m_State = state;

	return oldState;
}

void View::Resize(const UVector3& uSize)
{
	if (uSize.x == 0 || uSize.y == 0)
		return;

	m_Info.m_Size = uSize;

	VKRenderContext* pContext = m_pContext;

	if (m_pNativeTexture != nullptr)
	{
		m_pNativeTexture->Destroy();
		delete m_pNativeTexture;
	}

	m_pNativeTexture = new VKResource();

	const bool bIsDepth = m_Info.m_Type == E_DEPTH_STENCIL_VIEW;

	VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT |
	                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
	                          VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	usage |= bIsDepth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
	                  : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	const uint32_t uiDepth = m_Info.m_DimensionType == E_TEXTURE_3D
		? static_cast<uint32_t>(m_Info.m_Size.z) : 1u;

	if (!m_pNativeTexture->CreateImage(
			pContext->GetDevice(), pContext->GetAllocator(),
			VKImageType(m_Info.m_DimensionType),
			VKFormat(m_Info.m_ColorFormat),
			static_cast<uint32_t>(m_Info.m_Size.x),
			static_cast<uint32_t>(m_Info.m_Size.y),
			uiDepth, 1, usage))
	{
		delete m_pNativeTexture;
		m_pNativeTexture = nullptr;
		return;
	}

	m_pNativeTexture->SetDebugName(m_Info.m_Name);

	/* Create the view up front; passes ask for it every frame. */
	m_pNativeTexture->GetOrCreateImageView(
		m_Info.m_DimensionType == E_TEXTURE_3D ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D);

	for (TargetData& data : m_TargetData)
		CreateTarget(data);
}

void View::AddTarget(PRenderPass* pRenderPass, uint32_t uiID, Type type)
{
	bool bFound = false;

	for (TargetData& found : m_TargetData)
	{
		if (found.m_pTarget == pRenderPass)
		{
			bFound = true;
			break;
		}
	}

	TargetData data = { pRenderPass, uiID, type };

	if (!bFound)
		m_TargetData.push_back(data);

	CreateTarget(data);
}

void View::CreateTarget(TargetData& data)
{
	/* Nothing to build here under Vulkan: attachments are named in
	   VkRenderingInfo when the pass begins, and shader-resource descriptors
	   are written into the pass's descriptor set. Both read the image view
	   this View already owns. */
	VX_UNUSED(data);
}
