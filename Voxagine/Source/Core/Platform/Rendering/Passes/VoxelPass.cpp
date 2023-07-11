#include "pch.h"
#include "VoxelPass.h"

#include "Core/Platform/Rendering/Objects/Buffer.h"

#include "Core/Platform/Rendering/RenderDefines.h"

#include "Core/Platform/Rendering/RenderContextInc.h"
#include "Core/Platform/Rendering/Managers/ModelManagerInc.h"
#include "../../Platform.h"
#include <Core/Application.h>
#include "Core/Settings.h"

VoxelPass::VoxelPass(
	PRenderContext* pContext, Shader* pVertex, Shader* pPixel, Sampler* pSampler,
	Mapper* pVoxelMapper, Buffer* pCameraBuffer, Buffer* pAABBBuffer,
	View* pParticleTexture, View* pParticleDepthTexture
) : PRenderPass(pContext)
{
	// Creates a screen render target (for each buffer, m_uiFrameCount)
	RenderPass::Data RenderPassData;
	RenderPassData.m_Name = "Voxel";
	RenderPassData.m_TargetType = E_STATE_PIXEL_SHADER_RESOURCE;
	RenderPassData.m_TopologyType = E_PRIMITIVE_TOPOLOGY_TRIANGLE;
	RenderPassData.m_Topology = E_TOPOLOGY_TRIANGLESTRIP;
	RenderPassData.m_uiVertexCount = 14;
	RenderPassData.m_uiInstanceCount = 0;
	RenderPassData.m_fRenderScale = pContext->GetPlatform()->GetApplication()->GetSettings().GetResolutionScale();
	RenderPassData.m_pVertexShader = pVertex;
	RenderPassData.m_pPixelShader = pPixel;
	RenderPassData.m_bEnableDepth = true;
	RenderPassData.m_CullType = E_CULL_FRONT;
	RenderPassData.m_uiBackBuffers = 1;

	RenderPassData.m_DepthClearValue = 1.f;

	RenderPassData.m_Samplers.push_back(pSampler);

	RenderPassData.m_Mappers.push_back(pVoxelMapper);

	RenderPassData.m_Buffers.push_back(pCameraBuffer);
	RenderPassData.m_Buffers.push_back(pAABBBuffer);

	RenderPassData.m_Textures.push_back(pParticleTexture);
	RenderPassData.m_Textures.push_back(pParticleDepthTexture);

	RenderPassData.m_uiBindlessResourceCount = 1;

#ifdef _WINDOWS
	SetHeapManager(m_pContext->GetModelManager()->GetHeapManager());
#endif

	Init(RenderPassData);
}

void VoxelPass::Begin(PCommandEngine* pEngine)
{
	m_Data.m_uiInstanceCount = m_Data.m_Buffers.back()->GetInstanceCount();
	PRenderPass::Begin(pEngine);
}
