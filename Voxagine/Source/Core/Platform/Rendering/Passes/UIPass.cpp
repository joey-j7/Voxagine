#include "pch.h"

#include "Core/Platform/Rendering/Passes/UIPass.h"
#include "Core/Platform/Rendering/Objects/Buffer.h"

#include "Core/Platform/Rendering/RenderDefines.h"
#include "Core/Platform/Rendering/RenderContextInc.h"

UIPass::UIPass(
	PRenderContext* pContext, Shader* pVertex, Shader* pPixel, Sampler* pSamplerPoint,
	Buffer* pCameraBuffer, Buffer* pSpriteBuffer
) : PRenderPass(pContext)
{
	RenderPass::Data RenderPassData;
	RenderPassData.m_Name = "UI Renderer";
	RenderPassData.m_TargetType = E_STATE_PIXEL_SHADER_RESOURCE;
	RenderPassData.m_TopologyType = E_PRIMITIVE_TOPOLOGY_TRIANGLE;
	RenderPassData.m_Topology = E_TOPOLOGY_TRIANGLESTRIP;
	RenderPassData.m_uiVertexCount = 4;
	RenderPassData.m_uiInstanceCount = 0;
	RenderPassData.m_fRenderScale = 1.0f;
	RenderPassData.m_pVertexShader = pVertex;
	RenderPassData.m_pPixelShader = pPixel;
	RenderPassData.m_bEnableDepth = true;
	RenderPassData.m_CullType = E_CULL_NONE;
	RenderPassData.m_BlendEnabled = true;

	RenderPassData.m_uiBindlessResourceCount = 1;

#ifdef _WINDOWS
	SetHeapManager(m_pContext->GetTextureManager()->GetHeapManager());
#endif

	RenderPassData.m_Samplers.push_back(pSamplerPoint);

	RenderPassData.m_Buffers.push_back(pCameraBuffer);
	RenderPassData.m_Buffers.push_back(pSpriteBuffer);

	RenderPassData.m_BlendEnabled = true;

	Init(RenderPassData);
}

void UIPass::Begin(PCommandEngine* pEngine)
{
	m_Data.m_uiInstanceCount = static_cast<uint32_t>(m_Data.m_Buffers[1]->GetInstanceCount());
	PRenderPass::Begin(pEngine);
}
