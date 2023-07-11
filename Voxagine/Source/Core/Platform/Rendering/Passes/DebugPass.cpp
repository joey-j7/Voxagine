#include "pch.h"

#include "Core/Platform/Rendering/Passes/DebugPass.h"
#include "Core/Platform/Rendering/Objects/Buffer.h"

#include "Core/Platform/Rendering/RenderDefines.h"
#include "Core/Platform/Rendering/RenderContextInc.h"

DebugPass::DebugPass(
	PRenderContext* pContext, Shader* pVertex, Shader* pPixel,
	Buffer* pCameraBuffer, Buffer* pLineBuffer
) : PRenderPass(pContext)
{
	// Creates a screen render target (for each buffer, m_uiFrameCount)
	RenderPass::Data RenderPassData;
	RenderPassData.m_Name = "Debug Renderer";
	RenderPassData.m_TargetType = E_STATE_PIXEL_SHADER_RESOURCE;
	RenderPassData.m_TopologyType = E_PRIMITIVE_TOPOLOGY_LINE;
	RenderPassData.m_Topology = E_TOPOLOGY_LINELIST;
	RenderPassData.m_uiVertexCount = 2;
	RenderPassData.m_uiInstanceCount = 0;
	RenderPassData.m_fRenderScale = 1.0f;
	RenderPassData.m_pVertexShader = pVertex;
	RenderPassData.m_pPixelShader = pPixel;
	RenderPassData.m_bEnableDepth = true;
	RenderPassData.m_CullType = E_CULL_NONE;

	RenderPassData.m_Buffers.push_back(pCameraBuffer);
	RenderPassData.m_Buffers.push_back(pLineBuffer);

	Init(RenderPassData);
}

void DebugPass::Begin(PCommandEngine* pEngine)
{
#if defined(_DEBUG) || defined(EDITOR)
	m_Data.m_uiInstanceCount = static_cast<uint32_t>(m_pContext->m_DebugDrawLines.size() * 0.5f);
#endif

	PRenderPass::Begin(pEngine);
}
