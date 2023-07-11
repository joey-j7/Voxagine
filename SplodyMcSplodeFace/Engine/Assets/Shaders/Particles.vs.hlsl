#include "Defines.hlsl"

static const float3 vertices[] =
{
    float3(-0.5,	-0.5,	0.5),    // Front Bottom Left
    float3(0.5,		-0.5,	0.5),    // Front Bottom Right
    float3(-0.5,	0.5,	0.5),    // Front Top Left
    float3(0.5,		0.5,	0.5),    // Front Top Right
    float3(-0.5,	0.5,	-0.5),   // Back Top Left
    float3(0.5,		0.5,	-0.5),   // Back Top Right
    float3(-0.5,	-0.5,	-0.5),   // Back Bottom Left
    float3(0.5,		-0.5,	-0.5),   // Back Bottom Right
};

static int indices[24] =
{
    0, 1, 2, 3,
	5, 4, 7, 6,
	4, 0, 6, 2,
	1, 5, 3, 7,
	4, 5, 0, 1,
	2, 3, 6, 7
};


static float3 normals[6] =
{
    float3(0.0, 	0.0, 	1.0),
    float3(0.0, 	0.0, 	-1.0),
    float3(-1.0, 	0.0, 	0.0),
    float3(1.0, 	0.0, 	0.0),
    float3(0.0, 	1.0, 	0.0),
    float3(0.0, 	-1.0, 	0.0)
};

CONSTANT_BUFFER Data : register(b0)
{
    matrix mvp;
    matrix mv;
	
    float4 camPosition;
    float4 camOffset;
	
	float4 viewport;
    float4 lightDirection;
	
    uint4 worldSize;
	
	uint sdfCount;
	uint particleCount;

    uint lightCount;
    float padding;
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
	
	OUT.Color.r = float(particle.VoxelColor >> 0  & 255) / (255.0 + IDinst % 4);
	OUT.Color.g = float(particle.VoxelColor >> 8  & 255) / (255.0 + IDinst % 4);
	OUT.Color.b = float(particle.VoxelColor >> 16 & 255) / (255.0 + IDinst % 4); 
	
	float fDifference = max(0.0, clamp(dot(normals[IDvert / 4], -lightDirection.xyz), 0.0, 1.0));
	OUT.Color *= (AMBIENT_VALUE + fDifference);
	
    float4 pos = float4(particle.Position + vertices[indices[IDvert]], 1.0);
	OUT.Position = mul(mvp, pos);
	OUT.Color.a = 1.0; //distance(OUT.Position, camPosition.xyz) / 255.0; 
    OUT.PosVert = pos;
	
    return OUT;
}