#include "Defines.hlsl"

CONSTANT_BUFFER bakeCommand : register(b0)
{
    float4 Origin;
    float4 Scale;
    
    float4 Forward;
    float4 Right;
    float4 Up;
    
    float4 RoundedScale;
    uint4 WorldSize;

	uint MapperID;
    uint OverrideColor;

    uint VoxelCount;

    bool Padding[2];
};

VOXEL_RW_BUFFER voxelWorldData : register(u0);
VOXEL_BUFFER modelData[] : register(t0);

[numthreads(1, 1, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint uiDispatchID = dispatchID.x + dispatchID.y * 65534;

    VOXEL_BUFFER model = modelData[MapperID];

    // Translation
    VOXEL_FORMAT voxelPosition = model[uiDispatchID] * 255.0;

    // Directions
    float3 forward = Forward;
    float3 right = Right;
    float3 up = Up;

    // Scale
    voxelPosition.xyz *= Scale.xyz;
    // voxelPosition += scaleOffset;

    // World space + rotation
    uint3 worldPosition = uint3(
        uint(Origin.x + (forward.x * voxelPosition.z) + (right.x * voxelPosition.x) + (up.x * voxelPosition.y)),
        uint(Origin.y + (forward.y * voxelPosition.z) + (right.y * voxelPosition.x) + (up.y * voxelPosition.y)),
        uint(Origin.z + (forward.z * voxelPosition.z) + (right.z * voxelPosition.x) + (up.z * voxelPosition.y))
    );

    if (
        worldPosition.x >= WorldSize.x ||
        worldPosition.y >= WorldSize.y ||
        worldPosition.z >= WorldSize.z
    )
        return;

    // Retrieve resulting color
	// RenderState rendererState = pRenderer->GetState();

    // color = bHasOverrideColor ?
    // 	(overrideColor.inst.Color | static_cast<unsigned char>(rendererState + 1) << 24) :
    // 	(pair.second.inst.Color | static_cast<unsigned char>(rendererState + 1) << 24)
    // ;

    // Bake color into world through ModifyVoxel
    uint uiWorldID = worldPosition.x + worldPosition.y * WorldSize.x + worldPosition.z * WorldSize.x * WorldSize.y;

    if (voxelWorldData[uiWorldID].a == 0.0)
    {
        voxelWorldData[uiWorldID] = OverrideColor != 0 ? float4(1.0, 0.0, 0.0, 1.0) : model[VoxelCount + uiDispatchID.x];
        return;
    }

    return;
}