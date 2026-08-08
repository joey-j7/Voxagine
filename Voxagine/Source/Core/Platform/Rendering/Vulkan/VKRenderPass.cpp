#include "pch.h"
#include "VKRenderPass.h"

#include "Core/Platform/Rendering/Objects/Buffer.h"
#include "Core/Platform/Rendering/Objects/Mapper.h"
#include "Core/Platform/Rendering/Objects/Sampler.h"
#include "Core/Platform/Rendering/Objects/Shader.h"
#include "Core/Platform/Rendering/Objects/View.h"

#include "Core/Platform/Rendering/Vulkan/VKCommandEngine.h"
#include "Core/Platform/Rendering/Vulkan/VKPassBindings.h"
#include "Core/Platform/Rendering/Vulkan/VKRenderContext.h"
#include "Core/Platform/Rendering/Vulkan/VKTranslate.h"
#include "Core/Platform/Rendering/Vulkan/Managers/VKTextureManager.h"

#include <cstdio>

VKRenderPass::VKRenderPass(PRenderContext* pContext) : RenderPass(pContext)
{
	m_pDevice = pContext->GetDevice();
}

VKRenderPass::VKRenderPass(PRenderContext* pContext, const Data& data) : RenderPass(pContext)
{
	m_pDevice = pContext->GetDevice();
	Init(data);
}

VKRenderPass::~VKRenderPass()
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

void VKRenderPass::Init(const Data& data)
{
	m_Data = data;
	m_fInvRenderScale = 1.0f / m_Data.m_fRenderScale;

	m_Bindings = VKBuildRenderPassBindings(m_Data);

	if (!m_Bindings.empty())
	{
		VKApplyBindings(m_DescriptorLayout, m_Bindings);

		/* One set per frame in flight, so a set being written this frame is
		   never the one the GPU is still reading. */
		if (!m_DescriptorLayout.Build(m_pDevice, VKCommandEngine::m_uiFrameCount))
		{
			fprintf(stderr, "[vulkan] '%s' descriptor layout failed\n", m_Data.m_Name.c_str());
		}
		else
		{
			for (uint32_t i = 0; i < VKCommandEngine::m_uiFrameCount; ++i)
			{
				m_DescriptorSets[i] = m_DescriptorLayout.Allocate();

				if (m_DescriptorSets[i] == VK_NULL_HANDLE)
					fprintf(stderr, "[vulkan] '%s' descriptor set %u failed\n", m_Data.m_Name.c_str(), i);
			}
		}
	}

	if (!CreateAttachments())
		fprintf(stderr, "[vulkan] '%s' attachments failed\n", m_Data.m_Name.c_str());

	if (!CreatePipeline())
		fprintf(stderr, "[vulkan] '%s' pipeline failed\n", m_Data.m_Name.c_str());
}

bool VKRenderPass::CreateAttachments()
{
	UVector2 size = m_Data.m_bUseScreenResolution
		? m_pContext->GetRenderResolution()
		: UVector2(static_cast<uint32_t>(m_Data.m_TargetSize.x),
		           static_cast<uint32_t>(m_Data.m_TargetSize.y));

	size.x = static_cast<uint32_t>(size.x * m_Data.m_fRenderScale);
	size.y = static_cast<uint32_t>(size.y * m_Data.m_fRenderScale);

	if (size.x == 0 || size.y == 0)
		return false;

	m_TargetSize = size;

	/* One view per render target, times the back buffer count. The engine
	   indexes them as i + backBuffer * renderViewCount. */
	const uint32_t uiViews = m_Data.m_uiRenderViewCount * (m_Data.m_uiBackBuffers + 1);

	m_pTargetViews.clear();
	m_pTargetViews.reserve(uiViews);

	for (uint32_t i = 0; i < uiViews; ++i)
	{
		View::Info info;
		info.m_Name = m_Data.m_Name + " Target " + std::to_string(i);
		info.m_Size = UVector3(size.x, size.y, 1);
		info.m_DimensionType = E_TEXTURE_2D;
		info.m_ColorFormat = m_Data.m_TargetFormat[i % m_Data.m_TargetFormat.size()];
		info.m_State = E_STATE_PIXEL_SHADER_RESOURCE;
		info.m_Type = View::E_RENDER_TARGET_VIEW;

		m_pTargetViews.push_back(std::make_unique<View>(m_pContext, info));
		m_pTargetViews.back()->AddTarget(this, i, View::E_RENDER_TARGET_VIEW);
	}

	if (m_Data.m_bEnableDepth)
	{
		View::Info info;
		info.m_Name = m_Data.m_Name + " Depth";
		info.m_Size = UVector3(size.x, size.y, 1);
		info.m_DimensionType = E_TEXTURE_2D;
		info.m_ColorFormat = m_Data.m_DepthFormat;
		info.m_State = E_STATE_DEPTH_WRITE;
		info.m_Type = View::E_DEPTH_STENCIL_VIEW;

		m_pDepthView = std::make_unique<View>(m_pContext, info);
		m_pDepthView->AddTarget(this, 0, View::E_DEPTH_STENCIL_VIEW);
	}

	return true;
}

bool VKRenderPass::CreatePipeline()
{
	if (m_Data.m_pVertexShader == nullptr || m_Data.m_pPixelShader == nullptr)
		return false;

	PShader* pVertex = m_Data.m_pVertexShader->GetNative();
	PShader* pPixel = m_Data.m_pPixelShader->GetNative();

	if (pVertex == nullptr || pPixel == nullptr || !pVertex->IsValid() || !pPixel->IsValid())
		return false;

	VkDescriptorSetLayout setLayout = m_DescriptorLayout.GetLayout();

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = setLayout != VK_NULL_HANDLE ? 1 : 0;
	layoutInfo.pSetLayouts = setLayout != VK_NULL_HANDLE ? &setLayout : nullptr;

	if (vkCreatePipelineLayout(m_pDevice->Get(), &layoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
		return false;

	VkPipelineShaderStageCreateInfo stages[2]{};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = pVertex->GetModule();
	stages[0].pName = "main";

	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = pPixel->GetModule();
	stages[1].pName = "main";

	/* No vertex buffers. The DX12 path bound an empty vertex and index view
	   and called DrawInstanced; every shader builds its vertices from
	   SV_VertexID and structured buffers. */
	VkPipelineVertexInputStateCreateInfo vertexInput{};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VKTopology(m_Data.m_Topology);

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo raster{};
	raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode = VKCullMode(m_Data.m_CullType);
	/* Draw uses a negative-height viewport to keep D3D clip space, so the
	   winding a D3D pipeline saw is preserved and D3D's default front face
	   (clockwise) carries over with it. CCW here plus front-culling threw
	   away every face of the voxel pass's boxes. */
	raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
	raster.lineWidth = 1.f;

	VkPipelineMultisampleStateCreateInfo multisample{};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = m_Data.m_bEnableDepth ? VK_TRUE : VK_FALSE;
	depthStencil.depthWriteEnable = m_Data.m_bEnableDepth ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(m_Data.m_uiRenderViewCount);

	for (VkPipelineColorBlendAttachmentState& attachment : blendAttachments)
	{
		attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		attachment.blendEnable = m_Data.m_BlendEnabled ? VK_TRUE : VK_FALSE;
		attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		attachment.colorBlendOp = VK_BLEND_OP_ADD;
		attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	VkPipelineColorBlendStateCreateInfo blend{};
	blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
	blend.pAttachments = blendAttachments.data();

	/* Viewport and scissor follow the render target, which resizes with the
	   window, so keep them dynamic rather than rebuilding the pipeline. */
	const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamic{};
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic.dynamicStateCount = 2;
	dynamic.pDynamicStates = dynamicStates;

	std::vector<VkFormat> colorFormats;
	colorFormats.reserve(m_Data.m_uiRenderViewCount);

	for (uint32_t i = 0; i < m_Data.m_uiRenderViewCount; ++i)
		colorFormats.push_back(VKFormat(m_Data.m_TargetFormat[i % m_Data.m_TargetFormat.size()]));

	/* Dynamic rendering: the pipeline names its attachment formats directly
	   instead of needing a compatible VkRenderPass object built first. */
	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
	renderingInfo.pColorAttachmentFormats = colorFormats.data();
	renderingInfo.depthAttachmentFormat = m_Data.m_bEnableDepth
		? VKFormat(m_Data.m_DepthFormat) : VK_FORMAT_UNDEFINED;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = &renderingInfo;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = stages;
	pipelineInfo.pVertexInputState = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &raster;
	pipelineInfo.pMultisampleState = &multisample;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &blend;
	pipelineInfo.pDynamicState = &dynamic;
	pipelineInfo.layout = m_PipelineLayout;

	if (vkCreateGraphicsPipelines(m_pDevice->Get(), VK_NULL_HANDLE, 1,
	                              &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] vkCreateGraphicsPipelines failed for '%s'\n", m_Data.m_Name.c_str());
		return false;
	}

	return true;
}

View* VKRenderPass::GetTargetView(uint32_t i) const
{
	const uint32_t uiIndex = i + m_uiCurrentBackBuffer * m_Data.m_uiRenderViewCount;

	if (uiIndex >= m_pTargetViews.size())
		return nullptr;

	return m_pTargetViews[uiIndex].get();
}

View* VKRenderPass::GetDepthView() const
{
	return m_pDepthView.get();
}

UVector2 VKRenderPass::GetTargetSize() const
{
	return m_TargetSize;
}

void VKRenderPass::Resize(UVector2 uSize)
{
	if (uSize.x == 0 || uSize.y == 0)
		return;

	vkDeviceWaitIdle(m_pDevice->Get());

	m_TargetSize = uSize;

	const UVector3 size(uSize.x, uSize.y, 1);

	for (std::unique_ptr<View>& pView : m_pTargetViews)
		pView->Resize(size);

	if (m_pDepthView)
		m_pDepthView->Resize(size);
}

void VKRenderPass::ForceBegin(PCommandEngine* pEngine)
{
	const uint32_t uiSavedVertices = m_Data.m_uiVertexCount;
	const uint32_t uiSavedInstances = m_Data.m_uiInstanceCount;

	/* Begin bails when the pass has nothing to draw; a clear still needs the
	   attachments opened. */
	m_Data.m_uiVertexCount = m_Data.m_uiVertexCount > 0 ? m_Data.m_uiVertexCount : 1;
	m_Data.m_uiInstanceCount = m_Data.m_uiInstanceCount > 0 ? m_Data.m_uiInstanceCount : 1;

	Begin(pEngine);

	m_Data.m_uiVertexCount = uiSavedVertices;
	m_Data.m_uiInstanceCount = uiSavedInstances;
}

void VKRenderPass::Begin(PCommandEngine* pEngine)
{
	const bool bHasIndices = m_Data.m_uiIndexCount > 0;
	const bool bHasVertices = m_Data.m_uiVertexCount > 0;
	const bool bHasInstances = m_Data.m_uiInstanceCount > 0;

	if (!m_bIsDrawn && (!bHasInstances || (!bHasVertices && !bHasIndices)))
		return;

	if (m_Pipeline == VK_NULL_HANDLE)
		return;

	/* Never nest, including across passes: the flag lives on the engine
	   because dynamic rendering is a command buffer property. */
	if (pEngine->IsRenderingOpen())
		return;

	/* Every image this pass samples has to be readable before rendering opens,
	   because barriers cannot be issued inside a render pass instance. A target
	   belonging to a pass that has not drawn yet is still UNDEFINED, so this
	   cannot be left to whoever produced it. */
	for (const VKPassBinding& binding : m_Bindings)
	{
		if (binding.m_Kind != VKPassBinding::E_SAMPLED_IMAGE)
			continue;

		View* pInput = nullptr;

		if (binding.m_Source == VKPassBinding::E_SOURCE_PASS)
		{
			PRenderPass* pSourcePass =
				const_cast<PRenderPass*>(static_cast<const PRenderPass*>(binding.m_pSource));

			if (pSourcePass != nullptr)
				pInput = pSourcePass->GetTargetView(binding.m_uiViewIndex);
		}
		else if (binding.m_Source == VKPassBinding::E_SOURCE_VIEW)
		{
			pInput = const_cast<View*>(static_cast<const View*>(binding.m_pSource));
		}

		/* Skip anything this pass also renders to; that is handled below. */
		bool bIsOwnTarget = false;

		for (const std::unique_ptr<View>& pTarget : m_pTargetViews)
		{
			if (pTarget.get() == pInput)
			{
				bIsOwnTarget = true;
				break;
			}
		}

		if (pInput != nullptr && !bIsOwnTarget)
			pInput->SetState(pEngine, E_STATE_PIXEL_SHADER_RESOURCE);
	}

	/* Move every attachment into its rendering layout before the pass opens. */
	for (uint32_t i = 0; i < m_Data.m_uiRenderViewCount; ++i)
	{
		View* pView = GetTargetView(i);

		if (pView != nullptr)
			pView->SetState(pEngine, E_STATE_RENDER_TARGET);
	}

	if (m_pDepthView)
		m_pDepthView->SetState(pEngine, E_STATE_DEPTH_WRITE);

	pEngine->ApplyBarriers();

	std::vector<VkRenderingAttachmentInfo> colorAttachments;
	colorAttachments.reserve(m_Data.m_uiRenderViewCount);

	for (uint32_t i = 0; i < m_Data.m_uiRenderViewCount; ++i)
	{
		View* pView = GetTargetView(i);

		if (pView == nullptr || pView->GetNative() == nullptr)
			continue;

		VkRenderingAttachmentInfo attachment{};
		attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		attachment.imageView = pView->GetNative()->GetOrCreateImageView(VK_IMAGE_VIEW_TYPE_2D);
		attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.loadOp = m_Data.m_bClearPerFrame ? VK_ATTACHMENT_LOAD_OP_CLEAR
		                                            : VK_ATTACHMENT_LOAD_OP_LOAD;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.clearValue.color = {{ m_Data.m_ClearColor.r, m_Data.m_ClearColor.g,
		                                 m_Data.m_ClearColor.b, m_Data.m_ClearColor.a }};

		colorAttachments.push_back(attachment);
	}

	VkRenderingAttachmentInfo depthAttachment{};

	if (m_pDepthView && m_pDepthView->GetNative() != nullptr)
	{
		depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachment.imageView = m_pDepthView->GetNative()->GetOrCreateImageView(VK_IMAGE_VIEW_TYPE_2D);
		depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.loadOp = m_Data.m_bClearPerFrame ? VK_ATTACHMENT_LOAD_OP_CLEAR
		                                                 : VK_ATTACHMENT_LOAD_OP_LOAD;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.clearValue.depthStencil.depth = m_Data.m_DepthClearValue;
	}

	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea.extent = { m_TargetSize.x, m_TargetSize.y };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
	renderingInfo.pColorAttachments = colorAttachments.data();
	renderingInfo.pDepthAttachment = m_pDepthView ? &depthAttachment : nullptr;

	m_uiTimestampBeginIndex = pEngine->WriteTimestampBegin();

	vkCmdBeginRendering(pEngine->GetCommandBuffer(), &renderingInfo);

	m_bIsRendering = true;
	pEngine->SetRenderingOpen(true);
}

bool VKRenderPass::WriteDescriptors(PCommandEngine* pEngine, VkDescriptorSet set)
{
	std::vector<VkWriteDescriptorSet> writes;
	std::vector<VkDescriptorBufferInfo> bufferInfos;
	std::vector<VkDescriptorImageInfo> imageInfos;
	std::vector<VkBufferView> texelViews;

	/* Reserved up front: the infos are referenced by pointer from the writes,
	   so reallocation while filling them would dangle. */
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

		switch (binding.m_Kind)
		{
		case VKPassBinding::E_CONSTANT_BUFFER:
		case VKPassBinding::E_STORAGE_BUFFER:
		case VKPassBinding::E_STORAGE_IMAGE:
		{
			VkDescriptorBufferInfo info{};

			if (binding.m_Source == VKPassBinding::E_SOURCE_BUFFER)
			{
				const Buffer* pBuffer = static_cast<const Buffer*>(binding.m_pSource);

				if (pBuffer == nullptr || pBuffer->GetNativeHandle() == 0)
					continue;

				/* Buffer suballocates from the engine's upload pages, so the
				   descriptor is the page plus this frame's offset rather than
				   a resource of its own. */
				info.buffer = reinterpret_cast<VkBuffer>(pBuffer->GetNativeHandle());
				info.offset = pBuffer->GetNativeOffset();
				info.range = pBuffer->GetTotalSize() > 0 ? pBuffer->GetTotalSize() : VK_WHOLE_SIZE;
			}
			else if (binding.m_Source == VKPassBinding::E_SOURCE_MAPPER)
			{
				Mapper* pMapper = const_cast<Mapper*>(static_cast<const Mapper*>(binding.m_pSource));

				if (pMapper == nullptr || pMapper->GetNative() == nullptr)
					continue;

				info.buffer = pMapper->GetNative()->GetBuffer();
				info.offset = 0;
				info.range = VK_WHOLE_SIZE;
			}
			else
			{
				continue;
			}

			if (info.buffer == VK_NULL_HANDLE)
				continue;

			bufferInfos.push_back(info);

			write.descriptorType = binding.m_Kind == VKPassBinding::E_CONSTANT_BUFFER
				? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
				: VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write.pBufferInfo = &bufferInfos.back();
			break;
		}

		case VKPassBinding::E_UNIFORM_TEXEL_BUFFER:
		case VKPassBinding::E_STORAGE_TEXEL_BUFFER:
		{
			if (binding.m_Source != VKPassBinding::E_SOURCE_MAPPER)
				continue;

			Mapper* pMapper = const_cast<Mapper*>(static_cast<const Mapper*>(binding.m_pSource));

			if (pMapper == nullptr || pMapper->GetNative() == nullptr)
				continue;

			const VkBufferView view =
				pMapper->GetNative()->GetOrCreateBufferView(VKFormat(pMapper->GetInfo().m_ColorFormat));

			if (view == VK_NULL_HANDLE)
				continue;

			texelViews.push_back(view);

			write.descriptorType = binding.m_Kind == VKPassBinding::E_UNIFORM_TEXEL_BUFFER
				? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
				: VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
			write.pTexelBufferView = &texelViews.back();
			break;
		}

		case VKPassBinding::E_SAMPLED_IMAGE:
		{
			/* GetNative is non-const, and the binding table holds sources as
			   const void*. */
			View* pView = nullptr;

			if (binding.m_Source == VKPassBinding::E_SOURCE_PASS)
			{
				/* Resolved now rather than at Init: the source pass flips its
				   back buffer between frames, so the target view changes. */
				PRenderPass* pSourcePass =
					const_cast<PRenderPass*>(static_cast<const PRenderPass*>(binding.m_pSource));

				if (pSourcePass != nullptr)
					pView = pSourcePass->GetTargetView(binding.m_uiViewIndex);
			}
			else
			{
				pView = const_cast<View*>(static_cast<const View*>(binding.m_pSource));
			}

			if (pView == nullptr || pView->GetNative() == nullptr)
				continue;

			VkDescriptorImageInfo info{};
			info.imageView = pView->GetNative()->GetOrCreateImageView(VK_IMAGE_VIEW_TYPE_2D);
			info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			if (info.imageView == VK_NULL_HANDLE)
				continue;

			imageInfos.push_back(info);

			write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			write.pImageInfo = &imageInfos.back();
			break;
		}

		case VKPassBinding::E_SAMPLER:
		{
			const Sampler* pSampler = static_cast<const Sampler*>(binding.m_pSource);

			if (pSampler == nullptr || !pSampler->IsInitialized())
				continue;

			VkDescriptorImageInfo info{};
			info.sampler = *static_cast<VkSampler*>(const_cast<Sampler*>(pSampler)->GetNative());

			imageInfos.push_back(info);

			write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			write.pImageInfo = &imageInfos.back();
			break;
		}

		default:
			continue;
		}

		writes.push_back(write);
	}

	/* Bindless texture array, filled from the texture manager's live views so
	   sprites and fonts loaded at any point appear without the pass being
	   told. Model buffers (E_BINDLESS_SOURCE_MODELS) stay unwritten until the
	   VoxModel GPU upload is restored; the array is PARTIALLY_BOUND, so that
	   is legal as long as no shader indexes them. */
	std::vector<VkDescriptorImageInfo> bindlessInfos;

	if (m_Data.m_BindlessSource == RenderPass::E_BINDLESS_SOURCE_TEXTURES)
	{
		for (const VKPassBinding& binding : m_Bindings)
		{
			if (binding.m_Kind != VKPassBinding::E_BINDLESS_TEXTURES)
				continue;

			TextureManager* pManager = m_pContext->GetTextureManager();

			bindlessInfos.reserve(pManager->m_pViews.size());

			for (const auto& entry : pManager->m_pViews)
			{
				if (entry.first >= binding.m_uiCount)
					continue;

				View* pTexture = entry.second.get();

				/* A view that was never uploaded is still UNDEFINED; binding
				   it as sampled would be invalid. */
				if (pTexture == nullptr || pTexture->GetNative() == nullptr ||
				    pTexture->GetNative()->IsLayoutUndefined())
					continue;

				const VkImageView imageView =
					pTexture->GetNative()->GetOrCreateImageView(VK_IMAGE_VIEW_TYPE_2D);

				if (imageView == VK_NULL_HANDLE)
					continue;

				VkDescriptorImageInfo info{};
				info.imageView = imageView;
				info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				bindlessInfos.push_back(info);

				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = set;
				write.dstBinding = binding.m_uiBinding;
				write.dstArrayElement = entry.first;
				write.descriptorCount = 1;
				write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				write.pImageInfo = &bindlessInfos.back();

				writes.push_back(write);
			}
		}
	}

	if (!writes.empty())
	{
		vkUpdateDescriptorSets(m_pDevice->Get(), static_cast<uint32_t>(writes.size()),
		                       writes.data(), 0, nullptr);
	}

	VX_UNUSED(pEngine);

	/* Every binding the layout declares must have been written. Drawing with a
	   partially updated set means the shader samples undefined descriptors;
	   for the SDF marcher that is not a visual glitch but an unbounded loop,
	   which stalls the GPU long enough for the compositor to freeze. */
	/* The bindless array is declared PARTIALLY_BOUND, so leaving it unwritten
	   is legal as long as the shader does not index a slot that was never
	   filled. It holds the voxel model textures, which are populated as models
	   load, so requiring it here would block the pass from ever drawing. */
	size_t uiRequired = 0;

	for (const VKPassBinding& binding : m_Bindings)
	{
		if (binding.m_Kind != VKPassBinding::E_BINDLESS_TEXTURES)
			++uiRequired;
	}

	if (writes.size() < uiRequired && !m_bWarnedIncomplete)
	{
		/* Naming the binding is the difference between "this pass is blank"
		   and knowing which resource never arrived. */
		for (const VKPassBinding& binding : m_Bindings)
		{
			if (binding.m_Kind == VKPassBinding::E_BINDLESS_TEXTURES)
				continue;

			bool bWritten = false;

			for (const VkWriteDescriptorSet& write : writes)
			{
				if (write.dstBinding == binding.m_uiBinding)
				{
					bWritten = true;
					break;
				}
			}

			if (!bWritten)
			{
				fprintf(stderr, "[vulkan]   '%s' unwritten binding %u (kind %d, source %d): %s\n",
				        m_Data.m_Name.c_str(), binding.m_uiBinding,
				        static_cast<int>(binding.m_Kind), static_cast<int>(binding.m_Source),
				        binding.m_Name.c_str());
			}
		}
	}

	return writes.size() >= uiRequired;
}

void VKRenderPass::Draw(PCommandEngine* pEngine)
{
	const bool bHasIndices = m_Data.m_uiIndexCount > 0;
	const bool bHasVertices = m_Data.m_uiVertexCount > 0;
	const bool bHasInstances = m_Data.m_uiInstanceCount > 0;

	if (!bHasInstances || (!bHasVertices && !bHasIndices))
	{
		/* Also clear when the pass has never drawn. A pass skipped from the
		   very first frame - the UI pass with nothing to render - otherwise
		   never initialises its target, and a later pass samples undefined
		   memory. Post processing reads the UI target's alpha and returns
		   black when it is 1, so undefined contents there black out the frame. */
		if (m_bIsDrawn || !m_bIsCleared)
		{
			Clear(pEngine);

			m_bIsDrawn = false;
			m_bIsCleared = true;
		}

		return;
	}

	if (!m_bIsRendering || m_Pipeline == VK_NULL_HANDLE)
		return;

	VkCommandBuffer cmd = pEngine->GetCommandBuffer();

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

	/* Negative height flips Vulkan's Y-down clip space back to D3D's Y-up,
	   so the unmodified HLSL shaders render upright (core since 1.1). */
	VkViewport viewport{};
	viewport.y = static_cast<float>(m_TargetSize.y);
	viewport.width = static_cast<float>(m_TargetSize.x);
	viewport.height = -static_cast<float>(m_TargetSize.y);
	viewport.maxDepth = 1.f;

	VkRect2D scissor{};
	scissor.extent = { m_TargetSize.x, m_TargetSize.y };

	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	if (m_DescriptorLayout.IsBuilt())
	{
		VkDescriptorSet set = m_DescriptorSets[pEngine->GetFrameIndex() % VKCommandEngine::m_uiFrameCount];

		if (set == VK_NULL_HANDLE || !WriteDescriptors(pEngine, set))
		{
			/* Skip the draw rather than risk a GPU hang. Reported once so an
			   incomplete pass is visible instead of silently blank. */
			if (!m_bWarnedIncomplete)
			{
				fprintf(stderr, "[vulkan] '%s' has incomplete descriptors; skipping its draw\n",
				        m_Data.m_Name.c_str());
				m_bWarnedIncomplete = true;
			}

			return;
		}

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
		                        0, 1, &set, 0, nullptr);
	}

	vkCmdDraw(cmd, m_Data.m_uiVertexCount, m_Data.m_uiInstanceCount, 0, 0);

	m_bIsDrawn = true;
}

void VKRenderPass::End(PCommandEngine* pEngine)
{
	if (!m_bIsRendering || !pEngine->IsRenderingOpen())
		return;

	vkCmdEndRendering(pEngine->GetCommandBuffer());

	pEngine->WriteTimestampEnd(m_Data.m_Name, m_uiTimestampBeginIndex);
	m_uiTimestampBeginIndex = UINT32_MAX;

	m_bIsRendering = false;
	pEngine->SetRenderingOpen(false);

	/* Hand the targets back as shader resources so a later pass can sample
	   them without knowing what this one did. */
	for (uint32_t i = 0; i < m_Data.m_uiRenderViewCount; ++i)
	{
		View* pView = GetTargetView(i);

		if (pView != nullptr)
			pView->SetState(pEngine, E_STATE_PIXEL_SHADER_RESOURCE);
	}
}

void VKRenderPass::Clear(PCommandEngine* pEngine)
{
	if (!m_Data.m_bClearPerFrame || m_bIsCleared)
		return;

	/* The attachment load op already clears, so when rendering is open there
	   is nothing to do. Opening a second pass here is what produced
	   "vkCmdBeginRendering inside an active render pass instance". */
	if (pEngine->IsRenderingOpen())
		return;

	/* Skipped passes still need their targets cleared, which an empty
	   begin/end pair does through the load op. */
	ForceBegin(pEngine);
	End(pEngine);
}
