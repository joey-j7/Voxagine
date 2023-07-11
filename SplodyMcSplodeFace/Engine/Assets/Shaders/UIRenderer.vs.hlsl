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
};

struct TextData
{
	uint FontID;
	uint CharID;
	matrix Model;
};

STRUCTURED_BUFFER(SpriteData) Sprites : register(t0);
STRUCTURED_BUFFER(TextData) Text : register(t1);

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
	
	float3 normCoords = vertices[IDvert] - float3(AlignmentCoords[sprite.Alignment], 0.0);
	float3 spriteCoords = normCoords * float3(sprite.Size * 0.5, 1.0);
	
	if (sprite.IsScreen)
	{
		float2 screenCoords = normCoords.xy * sprite.Size * 0.5;
		screenCoords = mul(sprite.Model, float4(screenCoords, 0.0, 1.0)).xy;
		
		OUT.Position.xy = (screenCoords / (viewport.xy * 0.5) - float2(1.0, 1.0)) + (AlignmentCoords[sprite.ScreenAlignment] + float2(1.0, 1.0));
		OUT.Position.z = 0.0;
		OUT.Position.w = 1.0;
	}
	else
	{
		float3 worldCoords = float3(spriteCoords.xy, 0.0);
		OUT.Position = mul(mvp, mul(sprite.Model, float4(worldCoords, 1.0)));
	}
	
	OUT.UVs = (vertices[IDvert] + float2(1.0, 1.0)) * 0.5;
	OUT.UVs.y = 1.0 - OUT.UVs.y;
	
	OUT.TextureID = sprite.TextureID;
	OUT.Color = sprite.Color;
	
	return OUT;
}