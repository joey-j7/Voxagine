#include "pch.h"

#include "Core/Platform/Rendering/Passes/ParticlePass.h"
#include "Core/Platform/Rendering/Objects/Buffer.h"

#include "Core/Platform/Rendering/RenderContextInc.h"

#include "../RenderContext.h"
#include "../../Platform.h"
#include <Core/Application.h>

ParticlePass::ParticlePass(
	PRenderContext* pContext, Shader* pVertex, Shader* pPixel, Buffer* pCameraBuffer, Mapper* pMapper, Sampler* pSamplerPoint
) : PRenderPass(pContext)
{
	// Creates a screen render target (for each buffer, m_uiFrameCount)
	RenderPass::Data RenderPassData;
	RenderPassData.m_Name = "Particles";
	RenderPassData.m_TargetType = E_STATE_PIXEL_SHADER_RESOURCE;
	RenderPassData.m_uiRenderViewCount = 2;
	RenderPassData.m_TargetFormat.push_back(E_R32_FLOAT);
	RenderPassData.m_TopologyType = E_PRIMITIVE_TOPOLOGY_TRIANGLE;
	RenderPassData.m_Topology = E_TOPOLOGY_TRIANGLELIST;
	RenderPassData.m_uiVertexCount = 24;
	RenderPassData.m_uiInstanceCount = 0;
	RenderPassData.m_fRenderScale = pContext->GetPlatform()->GetApplication()->GetSettings().GetResolutionScale();
	RenderPassData.m_pVertexShader = pVertex;
	RenderPassData.m_pPixelShader = pPixel;
	RenderPassData.m_bEnableDepth = true;
	RenderPassData.m_CullType = E_CULL_NONE;

	RenderPassData.m_Buffers.push_back(pCameraBuffer);
	RenderPassData.m_Mappers.push_back(pMapper);

	RenderPassData.m_Samplers.push_back(pSamplerPoint);

	Init(RenderPassData);
}

void ParticlePass::Begin(PCommandEngine* pEngine)
{
	m_Data.m_uiInstanceCount = m_pContext->m_uiParticleCount;
	PRenderPass::Begin(pEngine);
}
