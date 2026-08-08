#include "pch.h"
#include "VKImContext.h"

#include "Core/Platform/Rendering/Objects/Shader.h"
#include "Core/Platform/Rendering/Objects/View.h"
#include "Core/Platform/Rendering/Vulkan/VKCommandEngine.h"
#include "Core/Platform/Rendering/Vulkan/VKRenderContext.h"
#include "Core/Platform/Rendering/Vulkan/VKResource.h"
#include "Core/Platform/Rendering/Vulkan/VKShaderBindings.h"
#include "Core/Platform/Rendering/Vulkan/VKTranslate.h"
#include "Core/Platform/Rendering/Vulkan/Managers/VKTextureManager.h"

#include "External/imgui/imgui.h"

#include <cstdio>
#include <cstring>

/* Everything here records into the render pass instance that is already open -
 * RenderContext::Present calls Draw inside the post processing pass - so this
 * never begins one of its own and its pipeline must declare that pass's
 * attachment format. */

namespace
{
	const char* const kVertexShaderPath = "Engine/Assets/Shaders/imgui.vs.spv";
	const char* const kPixelShaderPath = "Engine/Assets/Shaders/imgui.ps.spv";

	/* The pass ImGui draws inside; its target format has to match the pipeline. */
	const char* const kHostPassName = "Post Processing";
}

VKImContext::VKImContext(VKRenderContext* pContext) : m_pContext(pContext)
{
}

VKImContext::~VKImContext()
{
	Deinitialize();
}

void VKImContext::NewFrame()
{
	/* First frame, not the constructor: Platform builds this before
	   ImGui::CreateContext, so there is no ImGuiIO yet. And here rather than
	   in Draw, because the draw data already carries the atlas's TexID. */
	if (m_bFontsBuilt)
		return;

	m_bFontsBuilt = true;

	if (!BuildFontTexture())
	{
		fprintf(stderr, "[vulkan] imgui font atlas failed\n");

		/* ImGui::NewFrame asserts on a null TexID, so leave a non-null one
		   that resolves to no texture and skips its draw commands. */
		ImGui::GetIO().Fonts->TexID = reinterpret_cast<ImTextureID>(this);

		m_bInitFailed = true;
	}
}

bool VKImContext::BuildFontTexture()
{
	ImGuiIO& io = ImGui::GetIO();

	unsigned char* pPixels = nullptr;
	int iWidth = 0;
	int iHeight = 0;

	io.Fonts->GetTexDataAsRGBA32(&pPixels, &iWidth, &iHeight);

	if (pPixels == nullptr || iWidth <= 0 || iHeight <= 0)
		return false;

	/* The dedicated upload engine, so this cannot land in the middle of the
	   frame the Direct engine is recording. */
	PCommandEngine* pEngine = m_pContext->GetEngine("Texture");
	PTextureManager* pTextures = m_pContext->GetTextureManager();

	if (pEngine == nullptr || pTextures == nullptr)
		return false;

	/* Through the texture manager, so the atlas is a View like any sprite and
	   both resolve the same way in Draw. */
	const uint32_t uiID = pTextures->CreateTexture(pEngine, "ImGui Font Atlas", pPixels,
	                                               UVector2(static_cast<uint32_t>(iWidth),
	                                                        static_cast<uint32_t>(iHeight)));

	if (uiID == UINT32_MAX)
		return false;

	m_pFontTexture = pTextures->m_pViews[uiID].get();

	io.Fonts->TexID = static_cast<ImTextureID>(m_pFontTexture);

	/* ImGui keeps its own copy of the atlas; the GPU has it now. */
	io.Fonts->ClearTexData();

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

	return vkCreateSampler(m_pContext->GetDevice()->Get(), &samplerInfo, nullptr, &m_Sampler) == VK_SUCCESS;
}

bool VKImContext::BuildPipeline()
{
	Shader::Info vertexInfo;
	vertexInfo.m_FilePath = "Engine/Assets/Shaders/imgui.vs";
	vertexInfo.m_Type = Shader::E_VERTEX;

	Shader::Info pixelInfo;
	pixelInfo.m_FilePath = "Engine/Assets/Shaders/imgui.ps";
	pixelInfo.m_Type = Shader::E_PIXEL;

	/* Shader appends the .spv and reads the module; kept local because the
	   modules are only needed until the pipeline is created. */
	Shader vertexShader(m_pContext, vertexInfo);
	Shader pixelShader(m_pContext, pixelInfo);

	PShader* pVertex = vertexShader.GetNative();
	PShader* pPixel = pixelShader.GetNative();

	if (pVertex == nullptr || pPixel == nullptr || !pVertex->IsValid() || !pPixel->IsValid())
	{
		fprintf(stderr, "[vulkan] imgui shaders missing (%s, %s)\n",
		        kVertexShaderPath, kPixelShaderPath);
		return false;
	}

	/* The shader's b0/t0/s0, shifted the same way every other pass is. */
	m_DescriptorLayout.AddConstantBuffer(0, VK_SHADER_STAGE_VERTEX_BIT);
	m_DescriptorLayout.AddTexture(0, VK_SHADER_STAGE_FRAGMENT_BIT);
	m_DescriptorLayout.AddSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);

	/* Sets are allocated on demand; the pool only has to be sized for the
	   worst case. */
	if (!m_DescriptorLayout.Build(m_pContext->GetDevice(), m_uiFrameSlots * m_uiTexturesPerFrame))
		return false;

	VkDescriptorSetLayout setLayout = m_DescriptorLayout.GetLayout();

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &setLayout;

	if (vkCreatePipelineLayout(m_pContext->GetDevice()->Get(), &layoutInfo, nullptr,
	                           &m_PipelineLayout) != VK_SUCCESS)
	{
		return false;
	}

	VkPipelineShaderStageCreateInfo stages[2]{};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = pVertex->GetModule();
	stages[0].pName = "main";

	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = pPixel->GetModule();
	stages[1].pName = "main";

	/* The one pass in the engine with real vertex buffers: ImDrawVert is
	   interleaved position, uv and packed colour. DXC assigns locations in
	   declaration order, and the shader declares pos, col, uv - which is not
	   the order they sit in memory. */
	VkVertexInputBindingDescription binding{};
	binding.binding = 0;
	binding.stride = sizeof(ImDrawVert);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attributes[3]{};
	attributes[0].location = 0;
	attributes[0].binding = 0;
	attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributes[0].offset = offsetof(ImDrawVert, pos);

	attributes[1].location = 1;
	attributes[1].binding = 0;
	attributes[1].format = VK_FORMAT_R8G8B8A8_UNORM;
	attributes[1].offset = offsetof(ImDrawVert, col);

	attributes[2].location = 2;
	attributes[2].binding = 0;
	attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributes[2].offset = offsetof(ImDrawVert, uv);

	VkPipelineVertexInputStateCreateInfo vertexInput{};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &binding;
	vertexInput.vertexAttributeDescriptionCount = 3;
	vertexInput.pVertexAttributeDescriptions = attributes;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo raster{};
	raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode = VK_CULL_MODE_NONE;
	raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
	raster.lineWidth = 1.f;

	VkPipelineMultisampleStateCreateInfo multisample{};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	VkPipelineColorBlendAttachmentState blendAttachment{};
	blendAttachment.blendEnable = VK_TRUE;
	blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	                                 VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo blend{};
	blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend.attachmentCount = 1;
	blend.pAttachments = &blendAttachment;

	const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamic{};
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic.dynamicStateCount = 2;
	dynamic.pDynamicStates = dynamicStates;

	/* Must match the pass this draws inside, or the pipeline is incompatible
	   with the render pass instance already open. */
	PRenderPass* pHostPass = m_pContext->GetRenderPass(kHostPassName);

	if (pHostPass == nullptr)
		return false;

	const VkFormat colorFormat = VKFormat(pHostPass->GetData().m_TargetFormat[0]);

	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachmentFormats = &colorFormat;

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

	if (vkCreateGraphicsPipelines(m_pContext->GetDevice()->Get(), VK_NULL_HANDLE, 1,
	                              &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] imgui pipeline creation failed\n");
		return false;
	}

	return true;
}

VkDescriptorSet VKImContext::GetTextureSet(View* pTexture, const VkDescriptorBufferInfo& constants)
{
	if (pTexture == nullptr || pTexture->GetNative() == nullptr)
		return VK_NULL_HANDLE;

	std::vector<VkDescriptorSet>& sets = m_DescriptorSets[m_uiFrameSlot];

	/* One set per distinct texture, not per command: most commands in a
	   frame name the same atlas. */
	for (size_t i = 0; i < m_FrameTextures.size(); ++i)
	{
		if (m_FrameTextures[i] == pTexture)
			return sets[i];
	}

	if (m_FrameTextures.size() >= m_uiTexturesPerFrame)
	{
		static bool s_bWarned = false;

		if (!s_bWarned)
		{
			s_bWarned = true;
			fprintf(stderr, "[vulkan] imgui frame names more than %u textures; the rest will not draw\n",
			        m_uiTexturesPerFrame);
		}

		return VK_NULL_HANDLE;
	}

	VkImageView imageView = pTexture->GetNative()->GetOrCreateImageView(VK_IMAGE_VIEW_TYPE_2D);

	if (imageView == VK_NULL_HANDLE)
		return VK_NULL_HANDLE;

	if (sets.size() <= m_FrameTextures.size())
	{
		VkDescriptorSet allocated = m_DescriptorLayout.Allocate();

		if (allocated == VK_NULL_HANDLE)
			return VK_NULL_HANDLE;

		sets.push_back(allocated);
	}

	VkDescriptorSet set = sets[m_FrameTextures.size()];
	m_FrameTextures.push_back(pTexture);

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageView = imageView;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo samplerInfo{};
	samplerInfo.sampler = m_Sampler;

	VkWriteDescriptorSet writes[3]{};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].dstSet = set;
	writes[0].dstBinding = VKBindings::ConstantBuffer(0);
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &constants;

	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstSet = set;
	writes[1].dstBinding = VKBindings::Texture(0);
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	writes[1].pImageInfo = &imageInfo;

	writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[2].dstSet = set;
	writes[2].dstBinding = VKBindings::Sampler(0);
	writes[2].descriptorCount = 1;
	writes[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	writes[2].pImageInfo = &samplerInfo;

	vkUpdateDescriptorSets(m_pContext->GetDevice()->Get(), 3, writes, 0, nullptr);

	return set;
}

void VKImContext::Draw(ImDrawData* pDrawData)
{
	if (pDrawData == nullptr || pDrawData->CmdListsCount == 0 || m_bInitFailed)
		return;

	if (pDrawData->DisplaySize.x <= 0.f || pDrawData->DisplaySize.y <= 0.f)
		return;

	if (!m_bInitialised)
	{
		/* First draw: the pipeline declares the host pass's attachment
		   format, and that pass does not exist earlier. */
		if (!BuildPipeline())
		{
			fprintf(stderr, "[vulkan] imgui rendering disabled\n");
			m_bInitFailed = true;
			return;
		}

		m_bInitialised = true;
	}

	PCommandEngine* pEngine = m_pContext->GetEngine("Direct");

	if (pEngine == nullptr || !pEngine->IsRenderingOpen())
		return;

	VKUploadBuffer* pUpload = pEngine->GetUploadBuffer();

	if (pUpload == nullptr)
		return;

	const size_t uiVertexBytes = static_cast<size_t>(pDrawData->TotalVtxCount) * sizeof(ImDrawVert);
	const size_t uiIndexBytes = static_cast<size_t>(pDrawData->TotalIdxCount) * sizeof(ImDrawIdx);

	if (uiVertexBytes == 0 || uiIndexBytes == 0)
		return;

	/* Suballocated from the frame's upload pages, which are recycled once the
	   GPU has retired the frame that used them. */
	const VKUploadBuffer::Allocation vertexAlloc = pUpload->Allocate(uiVertexBytes, alignof(ImDrawVert));
	const VKUploadBuffer::Allocation indexAlloc = pUpload->Allocate(uiIndexBytes, alignof(ImDrawIdx));

	if (vertexAlloc.CPU == nullptr || indexAlloc.CPU == nullptr)
		return;

	ImDrawVert* pVertices = static_cast<ImDrawVert*>(vertexAlloc.CPU);
	ImDrawIdx* pIndices = static_cast<ImDrawIdx*>(indexAlloc.CPU);

	for (int i = 0; i < pDrawData->CmdListsCount; ++i)
	{
		const ImDrawList* pList = pDrawData->CmdLists[i];

		std::memcpy(pVertices, pList->VtxBuffer.Data,
		            static_cast<size_t>(pList->VtxBuffer.Size) * sizeof(ImDrawVert));
		std::memcpy(pIndices, pList->IdxBuffer.Data,
		            static_cast<size_t>(pList->IdxBuffer.Size) * sizeof(ImDrawIdx));

		pVertices += pList->VtxBuffer.Size;
		pIndices += pList->IdxBuffer.Size;
	}

	/* Maps ImGui's top-left pixel space straight onto Vulkan's Y-down clip
	   space, so this pass keeps a positive-height viewport rather than the
	   flipped one the engine's own passes use for their D3D-era shaders.
	   Stored column by column: DXC packs a cbuffer matrix column-major, which
	   is why the engine's glm matrices work with mul(M, v) unmodified. */
	const float fScaleX = 2.f / pDrawData->DisplaySize.x;
	const float fScaleY = 2.f / pDrawData->DisplaySize.y;

	const float projection[16] = {
		fScaleX, 0.f, 0.f, 0.f,
		0.f, fScaleY, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		-1.f - pDrawData->DisplayPos.x * fScaleX,
		-1.f - pDrawData->DisplayPos.y * fScaleY,
		0.f, 1.f
	};

	const VKUploadBuffer::Allocation constantAlloc = pUpload->AllocateConstant(sizeof(projection));

	if (constantAlloc.CPU == nullptr)
		return;

	std::memcpy(constantAlloc.CPU, projection, sizeof(projection));

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = constantAlloc.Buffer;
	bufferInfo.offset = constantAlloc.Offset;
	bufferInfo.range = sizeof(projection);

	/* Next frame's sets before writing any, so none is one the GPU is still
	   reading. */
	m_uiFrameSlot = (m_uiFrameSlot + 1) % m_uiFrameSlots;
	m_FrameTextures.clear();

	VkCommandBuffer cmd = pEngine->GetCommandBuffer();

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

	vkCmdBindVertexBuffers(cmd, 0, 1, &vertexAlloc.Buffer, &vertexAlloc.Offset);
	vkCmdBindIndexBuffer(cmd, indexAlloc.Buffer, indexAlloc.Offset,
	                     sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

	/* Positive height: ImGui works in Y-down pixels already, so unlike the
	   engine's passes this wants Vulkan's own clip space. */
	VkViewport viewport{};
	viewport.width = pDrawData->DisplaySize.x;
	viewport.height = pDrawData->DisplaySize.y;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(cmd, 0, 1, &viewport);

	uint32_t uiVertexOffset = 0;
	uint32_t uiIndexOffset = 0;

	for (int i = 0; i < pDrawData->CmdListsCount; ++i)
	{
		const ImDrawList* pList = pDrawData->CmdLists[i];

		for (int j = 0; j < pList->CmdBuffer.Size; ++j)
		{
			const ImDrawCmd& command = pList->CmdBuffer[j];

			/* This ImGui predates per-command index offsets: a list's
			   commands consume its indices in order, so this must advance
			   even when the command below is skipped. */
			const uint32_t uiFirstIndex = uiIndexOffset;
			uiIndexOffset += command.ElemCount;

			if (command.UserCallback != nullptr)
			{
				command.UserCallback(pList, &command);
				continue;
			}

			/* Clip rects are in ImGui's space; the scissor is in the
			   attachment's, and a negative offset is invalid. */
			const float fClipX = command.ClipRect.x - pDrawData->DisplayPos.x;
			const float fClipY = command.ClipRect.y - pDrawData->DisplayPos.y;
			const float fClipW = command.ClipRect.z - command.ClipRect.x;
			const float fClipH = command.ClipRect.w - command.ClipRect.y;

			if (fClipW <= 0.f || fClipH <= 0.f)
				continue;

			/* A command whose texture never loaded gets no set, and is
			   skipped rather than drawn with whatever was bound last. */
			VkDescriptorSet set = GetTextureSet(static_cast<View*>(command.TextureId), bufferInfo);

			if (set == VK_NULL_HANDLE)
				continue;

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
			                        0, 1, &set, 0, nullptr);

			VkRect2D scissor{};
			scissor.offset.x = static_cast<int32_t>(fClipX < 0.f ? 0.f : fClipX);
			scissor.offset.y = static_cast<int32_t>(fClipY < 0.f ? 0.f : fClipY);
			scissor.extent.width = static_cast<uint32_t>(fClipW);
			scissor.extent.height = static_cast<uint32_t>(fClipH);

			vkCmdSetScissor(cmd, 0, 1, &scissor);

			vkCmdDrawIndexed(cmd, command.ElemCount, 1, uiFirstIndex,
			                 static_cast<int32_t>(uiVertexOffset), 0);
		}

		uiVertexOffset += static_cast<uint32_t>(pList->VtxBuffer.Size);
	}
}

void VKImContext::Deinitialize()
{
	if (m_pContext == nullptr || m_pContext->GetDevice() == nullptr ||
		m_pContext->GetDevice()->Get() == VK_NULL_HANDLE)
	{
		return;
	}

	VkDevice device = m_pContext->GetDevice()->Get();

	vkDeviceWaitIdle(device);

	if (m_Pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(device, m_Pipeline, nullptr);
		m_Pipeline = VK_NULL_HANDLE;
	}

	if (m_PipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}

	/* Destroying the pool invalidates every set handed out of it. */
	m_DescriptorLayout.Destroy();

	for (std::vector<VkDescriptorSet>& sets : m_DescriptorSets)
		sets.clear();

	m_FrameTextures.clear();

	if (m_Sampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(device, m_Sampler, nullptr);
		m_Sampler = VK_NULL_HANDLE;
	}

	/* The font atlas is the texture manager's; it frees it with the rest. */
	m_pFontTexture = nullptr;

	m_bInitialised = false;
}
