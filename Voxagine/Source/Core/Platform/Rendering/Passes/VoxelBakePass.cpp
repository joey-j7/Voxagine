#include "pch.h"
#include "VoxelBakePass.h"

#include "Core/Platform/Rendering/RenderContextInc.h"

VoxelBakePass::VoxelBakePass(
	PRenderContext* pContext, Shader* pCompute,
	Mapper* pVoxelMapper, Buffer* pBakeCommandBuffer
) : PComputePass(pContext)
{
	// Creates a screen render target (for each buffer, m_uiFrameCount)
	ComputePass::Data ComputePassData;
	ComputePassData.m_Name = "Voxel Baker";
	ComputePassData.m_pShader = pCompute;

	ComputePassData.m_Buffers.push_back(pBakeCommandBuffer);
	ComputePassData.m_Mappers.push_back(pVoxelMapper);

	ComputePassData.m_uiBindlessResourceCount = 1;

#ifdef _WINDOWS
	SetHeapManager(m_pContext->GetModelManager()->GetHeapManager());
#endif

	Init(ComputePassData);
}