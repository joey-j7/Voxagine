#include "Defines.hlsl"

static float3 vertices[] =
{
    float3(-1.0,	-1.0,	0.0),   //	Bottom Left		(-1,	-1)
    float3( 1.0,	-1.0,	0.0),   //	Bottom Right	( 1, 	-1)
    float3(-1.0,	 1.0,	0.0),   //	Top Left		(-1,	 1)
    float3( 1.0,	 1.0,	0.0)    //	Top Right		( 1,	 1)
};

static float2 AlignmentCoords[9] =
{
	float2(0.0, 0.0),	// Centered
	float2(-1.0, 1.0),	// TopLeft
	float2(0.0, 1.0),	// TopCenter
	float2(1.0, 1.0),	// TopRight
	float2(1.0, 0.0),	// RightCenter
	float2(-1.0, 0.0),	// LeftCenter
	float2(-1.0, -1.0),	// BottomLeft
	float2(0.0, -1.0),	// BottomCenter
	float2(1.0, -1.0),	// BottomRight
};

// Register (b0)
#include "CameraData.hlsl"

struct SpriteData
{
	matrix Model;
	uint TextureID;
	
	float4 Color;
	
	float2 Offset;
	float2 Size;
	
	uint Alignment;
	uint ScreenAlignment;
	
	uint IsScreen;
	
	int Layer;
	
	float2 TextureRepeat; 
	
	float2 cullStart;
	float2 cullEnd;
	
	uint padding;
};

struct TextData
{
	uint FontID;
	uint CharID;
	matrix Model;
};

STRUCTURED_BUFFER(SpriteData) Sprites : register(t0);

struct VS_out
{
    float4	Position	: POS_OUT;
    float2	UVs			: TEXCOORD0;
    uint 	TextureID 	: POSITION;
	float4	Color		: COLOR;
};

VS_out main(uint IDvert : VERT_ID, uint IDinst : INST_ID)
{
	VS_out OUT;
	SpriteData sprite = Sprites[IDinst];
	
	// Convert cull start and end from 0,0 - 1,1 to -1,-1 - 1,1
	float2 cullStart = sprite.cullStart * 2 - 1;
	float2 cullEnd = sprite.cullEnd * 2 - 1;
	
	// Clamp vertex positions with culling.
	float3 vertexPos = vertices[IDvert];
	vertexPos.xy = clamp(vertexPos.xy, cullStart, cullEnd);
	
	float3 normCoords = vertexPos - float3(AlignmentCoords[sprite.Alignment], 0.0); // Setting the vertex position with correct center.
	float3 spriteCoords = normCoords * float3(sprite.Size * 0.5, 1.0); // Sizing mesh to the image size
	
	if (sprite.IsScreen)
	{
		float2 screenCoords = spriteCoords.xy;
		screenCoords = mul(sprite.Model, float4(screenCoords, 0.0, 1.0)).xy;
		
		OUT.Position.xy = (screenCoords / (viewport.xy * 0.5) - float2(1.0, 1.0)) + (AlignmentCoords[sprite.ScreenAlignment] + float2(1.0, 1.0));
		// Convert layer from -9999 - 9999 to 0 - 1 for depth.
		OUT.Position.z = ((float(sprite.Layer) * 0.0001) + 1) * 0.5;
		OUT.Position.w = 1.0;
	}
	else
	{
		float3 worldCoords = float3(spriteCoords.xy, 0.0);
		OUT.Position = mul(mvp, mul(sprite.Model, float4(worldCoords, 1.0)));
	}
	
	OUT.UVs = (vertexPos + float2(1.0, 1.0)) * 0.5; // Convert from -1,-1 - 1,1 To 0,0 - 1,1
	OUT.UVs.y = 1.0 - OUT.UVs.y; // Invert Y axis
	
	OUT.UVs *= sprite.TextureRepeat; // Repeat sprite
	
	OUT.TextureID = sprite.TextureID;
	OUT.Color = sprite.Color;
	
	return OUT;
}