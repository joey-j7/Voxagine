#include "pch.h"

#include "Core/Platform/Rendering/Passes/DepthPrepass.h"
#include "Core/Platform/Rendering/RenderPass.h"

#include "Core/Platform/Rendering/RenderContextInc.h"
#include "../../Platform.h"
#include <Core/Application.h>
#include "Core/Settings.h"

const float DepthPrepass::k_fScale = 0.125f;
const float DepthPrepass::k_fMiss = 1.0e9f;

DepthPrepass::DepthPrepass(
	PRenderContext* pContext, Shader* pVertex, Shader* pPixel,
	Buffer* pCameraBuffer, Mapper* pVoxelMapper, Mapper* pBrickMapper
) : PRenderPass(pContext)
{
	RenderPass::Data RenderPassData;
	RenderPassData.m_Name = "Depth Prepass";
	RenderPassData.m_TargetType = E_STATE_PIXEL_SHADER_RESOURCE;
	RenderPassData.m_TargetFormat = { E_R32_FLOAT };
	RenderPassData.m_uiVertexCount = 3;
	RenderPassData.m_pVertexShader = pVertex;
	RenderPassData.m_pPixelShader = pPixel;

	/* On top of the resolution scale the rest of the scene renders at, so the
	   prepass keeps its 1:k_fScale ratio to the voxel target however that is
	   set - which is what lets the consumer map pixel to texel by scaling
	   alone. */
	RenderPassData.m_fRenderScale =
		pContext->GetPlatform()->GetApplication()->GetSettings().GetResolutionScale() * k_fScale;

	/* No depth: one full-screen triangle, nothing to sort against. */
	RenderPassData.m_bEnableDepth = false;

	/* Cleared to a miss, so a frame in which the pass does not draw reads as
	   "no information" rather than as a skip past everything. */
	RenderPassData.m_ClearColor = Vector4(k_fMiss, k_fMiss, k_fMiss, k_fMiss);

	/* Order is the SPIR-V contract, same as VoxelPass: the mappers take u
	   registers in the order they are pushed, so the voxel buffer is u0 and the
	   brick counts u1, matching SDFPrepass.ps.hlsl. */
	RenderPassData.m_Buffers.push_back(pCameraBuffer);
	RenderPassData.m_Mappers.push_back(pVoxelMapper);
	RenderPassData.m_Mappers.push_back(pBrickMapper);

	Init(RenderPassData);
}

void DepthPrepass::Begin(PCommandEngine* pEngine)
{
	m_Data.m_uiInstanceCount = m_pContext->IsDepthPrepassEnabled() ? 1 : 0;
	PRenderPass::Begin(pEngine);
}
