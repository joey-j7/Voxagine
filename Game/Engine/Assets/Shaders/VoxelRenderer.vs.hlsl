#include "Defines.hlsl"
#include "CameraData.hlsl" // register(b0)

static const float3 vertices[] = {
    float3(-1.0,	 1.0,	 1.0),    // Front Top Left
    float3( 1.0,	 1.0,	 1.0),    // Front Top Right
    float3(-1.0,	-1.0,	 1.0),    // Front Bottom Left
    float3( 1.0,	-1.0,	 1.0),    // Front Bottom Right
    float3(-1.0,	 1.0,	-1.0),   // Back Top Left
    float3( 1.0,	 1.0,	-1.0),   // Back Top Right
    float3(-1.0,	-1.0,	-1.0),   // Back Bottom Left
    float3( 1.0,	-1.0,	-1.0),   // Back Bottom Right
};

static int indices[14] =
{
    3, 2, 1, 0,
	4, 2, 6, 3,
	7, 1, 5, 4,
	7, 6
};

static float3 CUBE_VERTS[24] = {
	float3(-1, 0, -1),	float3(0, -1, 0),	float3(-1, 0,  1),
	float3(-1, 0,  1),	float3(0, -1, 0),	float3( 1, 0,  1),
	float3(1,  0,  1),	float3(0, -1, 0),	float3( 1, 0, -1),
	float3(1,  0, -1),	float3(0, -1, 0),	float3(-1, 0, -1),
	float3(-1, 0, -1),	float3(0,  1, 0),	float3(-1, 0,  1),
	float3(-1, 0,  1),	float3(0,  1, 0),	float3( 1, 0,  1),
	float3(1,  0,  1),	float3(0,  1, 0),	float3( 1, 0, -1),
	float3(1,  0, -1),	float3(0,  1, 0),	float3(-1, 0, -1)
};

VOXEL_RW_BUFFER voxelWorldData : register(u0);

struct AABB
{
	float3 Position;
	float3 Extents;
	int TextureID;
	float Distance;
};

STRUCTURED_BUFFER(AABB) AABBs : register(t0);
Texture2D<float4> particlePass : register(t1);

VOXEL_BUFFER voxelModelData[] : register(t2) {};

struct VS_out
{
    float4 NormScreenPosition	: POS_OUT;
	float4 Direction			: POSITION0;
    float3 WorldPosition		: POSITION1;
};

VS_out main(uint IDvert : VERT_ID, uint IDinst : INST_ID)
{
    VS_out OUT;
	
    // Get relevant AABB data
    AABB tAABB = AABBs[IDinst];

    // Get cube model data
    // Store world-space coordinates in the vertex
	OUT.WorldPosition = vertices[indices[IDvert]] * tAABB.Extents + tAABB.Position;
	OUT.WorldPosition = clamp(OUT.WorldPosition.xyz, float3(0.0, 0.0, 0.0), float3(worldSize.xyz)) + camOffset;

    // Multiply the position with the MVP matrix
    OUT.NormScreenPosition = mul(mvp, float4(OUT.WorldPosition, 1.0));
	
	OUT.Direction = float4(OUT.WorldPosition.xyz - camPosition.xyz, 0.0);
	
    return OUT;
}