#include "pch.h"

#include "Core/Platform/Rendering/Passes/PostProcessingPass.h"
#include "Core/Platform/Rendering/RenderPass.h"

PostProcessingPass::PostProcessingPass(
	PRenderContext* pContext, Shader* pVertex, Shader* pPixel, Sampler* pSampler,
	Buffer* pCameraBuffer, std::vector<View*> pTextures
) : PRenderPass(pContext)
{
	// Creates a screen render target (for each buffer, m_uiFrameCount)
	RenderPass::Data RenderPassData;
	RenderPassData.m_Name = "Post Processing";
	RenderPassData.m_TargetType = E_STATE_PRESENT;
	RenderPassData.m_uiVertexCount = 3;
	RenderPassData.m_pVertexShader = pVertex;
	RenderPassData.m_fRenderScale = 1.0;
	RenderPassData.m_pPixelShader = pPixel;
	RenderPassData.m_bClearPerFrame = false;

#ifdef _ORBIS
	RenderPassData.m_TargetFormat = E_R8G8B8A8_UNORM_SRGB;
#endif

	RenderPassData.m_Buffers.push_back(pCameraBuffer);
	RenderPassData.m_Samplers.push_back(pSampler);

	for (View* pTexture : pTextures)
	{
		RenderPassData.m_Textures.push_back(pTexture);
	}

	Init(RenderPassData);
}