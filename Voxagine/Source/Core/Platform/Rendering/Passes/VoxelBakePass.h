#pragma once

#include "Core/Platform/Rendering/ComputePassInc.h"
#include "Core/Platform/Rendering/RenderDefines.h"

class Mapper;

class VoxelBakePass : public PComputePass
{
public:
	VoxelBakePass(
		PRenderContext* pContext, Shader* pCompute,
		Mapper* pVoxelMapper, Buffer* pBakeCommandBuffer
	);
};