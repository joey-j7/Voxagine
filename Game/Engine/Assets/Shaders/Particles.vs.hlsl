#include "Defines.hlsl"
#include "CameraData.hlsl" // register(b0)

static const float3 positions[8] =
{
    float3(-0.5,	-0.5,	0.5),    // Back Bottom Left
    float3(0.5,		-0.5,	0.5),    // Back Bottom Right
    float3(-0.5,	0.5,	0.5),    // Back Top Left
    float3(0.5,		0.5,	0.5),    // Back Top Right
    float3(-0.5,	0.5,	-0.5),   // Front Top Left
    float3(0.5,		0.5,	-0.5),   // Front Top Right
    float3(-0.5,	-0.5,	-0.5),   // Front Bottom Left
    float3(0.5,		-0.5,	-0.5)    // Front Bottom Right
};

static const int posIndices[36] =
{
    // Front Face
    5,   // Front Top Right
    6,   // Front Bottom Left
    4,   // Front Top Left
    
    5,   // Front Top Right
    7,   // Front Bottom Right
    6,   // Front Bottom Left

    // Top Face
    3,    // Back Top Right
    5,   // Front Top Right
    4,   // Front Top Left
    
    3,    // Back Top Right
    4,   // Front Top Left
    2,    // Back Top Left

    // Left Face
    6,   // Front Bottom Left
    0,    // Back Bottom Left
    2,    // Back Top Left

    6,   // Front Bottom Left
    2,    // Back Top Left
    4,   // Front Top Left
    
    // Right Face
    3,    // Back Top Right
    7,   // Front Bottom Right
    5,   // Front Top Right
    
    7,   // Front Bottom Right
    3,    // Back Top Right
    1,    // Back Bottom Right

    // Bottom Face
    1,    // Back Bottom Right
    6,   // Front Bottom Left
    7,   // Front Bottom Right
    
    1,    // Back Bottom Right
    0,    // Back Bottom Left
    6,   // Front Bottom Left

    // Back Face
    2,    // Back Top Left
    0,    // Back Bottom Left
    1,    // Back Bottom Right

    3,    // Back Top Right
    2,    // Back Top Left
    1    // Back Bottom Right
};

static float3 normals[6] =
{
    float3(0.0, 	0.0, 	1.0),       // Back Face
    float3(0.0, 	1.0, 	0.0),       // Top Face
    float3(-1.0, 	0.0, 	0.0),       // Left Face
    float3(1.0, 	0.0, 	0.0),       // Right Face
    float3(0.0, 	-1.0, 	0.0),       // Bottom Face
    float3(0.0, 	0.0, 	-1.0)       // Front Face
};

static int normIndices[36] =
{
    // Front Face
    5, 5, 5,
    5, 5, 5,

    // Top Face
    1, 1, 1,
    1, 1, 1,

    // Left Face
    2, 2, 2,
    2, 2, 2,
    
    // Right Face
    3, 3, 3,
    3, 3, 3,

    // Bottom Face
    4, 4, 4,
    4, 4, 4,

    // Back Face
    0, 0, 0,
    0, 0, 0
};

struct Particle
{
	float3 Position;
	uint VoxelColor;
};

STRUCTURED_BUFFER(Particle) Particles : register(t0);

struct VS_out
{
    float4 Color		: COLOR;
    float4 PosVert      : POSITION;
    float4 Position		: POS_OUT;
};

VS_out main(uint IDvert : VERT_ID, uint IDinst : INST_ID)
{
	VS_out OUT;
	
    //Get relevant SDF data
    Particle particle = Particles[IDinst];
	
    OUT.Color.r = float(particle.VoxelColor >> 0 & 255) / (255.0 + IDinst % 4);
    OUT.Color.g = float(particle.VoxelColor >> 8 & 255) / (255.0 + IDinst % 4);
    OUT.Color.b = float(particle.VoxelColor >> 16 & 255) / (255.0 + IDinst % 4);
	
	float fDifference = clamp(dot(normals[normIndices[IDvert]], -lightDirection.xyz), 0.0, 1.0);
    OUT.Color *= fDifference * (1.0 - AMBIENT_VALUE) + AMBIENT_VALUE;
	
    float4 pos = float4(particle.Position + positions[posIndices[IDvert]], 1.0);
	OUT.Position = mul(mvp, pos);
	OUT.Color.a = 1.0; //distance(OUT.Position, camPosition.xyz) / 255.0; 
    OUT.PosVert = pos;
	
    return OUT;
}