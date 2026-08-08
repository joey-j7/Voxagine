#include "Defines.hlsl"
#include "CameraData.hlsl" // register(b0)
#include "Camera.hlsl"

SamplerState s0 : register(s0);

Texture2D<float4> targetTexture : register(t0);
Texture2D<float4> uiTexture : register(t1);
Texture2D<float4> debugTexture : register(t2);

VOXEL_RW_BUFFER voxelWorldData : register(u0);

/* Far-field LOD volume and its occupancy bricks - see FarField.hlsl. Bound
   read-write for the same reason the window's brick counts are: the u range is
   what is free, and taking a t would renumber the textures above. Nothing
   writes to either from the GPU. */
VOXEL_RW_BUFFER farFieldData : register(u1);
RW_STRUCTURED_BUFFER(uint) farFieldBrickData : register(u2);

#ifndef __PSSL__
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 39
#include "FXAA3_11.hlsl"
#endif

/* Minimal standalone copy of SDFMarcher.hlsl's voxel lookup - just enough for
   the ground sample below. Not #include-ing SDFMarcher.hlsl itself since this
   pass has no march loop and doesn't need it. */
inline uint PostFxPosToVoxelID(uint3 v3Position) {
	return v3Position.x + v3Position.y * worldSize.x + worldSize.x * worldSize.y * v3Position.z;
}

inline float4 PostFxGetVoxel(float3 v3Position) {
	uint ID = PostFxPosToVoxelID(uint3(v3Position));
#ifdef __PSSL__
	uint uiColor = voxelWorldData[ID];
	return float4(0xFF & (uiColor), 0xFF & (uiColor >> 8), 0xFF & (uiColor >> 16), 0xFF & (uiColor >> 24)) / 255.0;
#else
	return voxelWorldData[ID];
#endif
}

/* Background for pixels the Voxel pass left untouched - either no AABB proxy
   covered them, or MarchDiffuse found nothing (see VoxelRenderer.ps.hlsl).
   Far-field geometry, endless ground and sky all come from GetBackground; this
   pass runs full-screen unconditionally, so unlike the proxy-box approach it
   always covers every pixel. */
#include "FarField.hlsl"

/* FXAA has to be kept away from the scene's silhouette against the background.
   The Voxel pass writes float4(0, 0, 0, 0) where its march found nothing, so a
   neighbourhood straddling a silhouette contains transparent *black* - and FXAA
   blends colour, not coverage, so it pulls the surface toward that black and
   leaves a one-pixel dark fringe along the edge. Against the sky that has
   always been there and read as an outline; along the resident window's
   boundary, where the ground continues on the far side at a matching colour, it
   reads as a seam across the middle of a flat plane.

   Nothing is lost by skipping it there. FXAA smooths high-contrast edges
   *within* the scene, and a pixel on the silhouette is composited against the
   background immediately below - there is no second scene sample to blend with.

   Load with clamped coordinates rather than Sample with offsets: R_DEF_WRAP_MODE
   is E_WRAP, so a neighbour offset past the screen edge would answer with a
   pixel from the far side. */
bool IsSceneNeighbourhoodOpaque(float2 v2PixelPosition)
{
	int2 v2Size = int2(max(viewport.xy, float2(1.0, 1.0)));
	int2 v2Texel = int2(v2PixelPosition);

	[unroll]
	for (int y = -1; y <= 1; y++)
	{
		[unroll]
		for (int x = -1; x <= 1; x++)
		{
			int2 v2Sample = clamp(v2Texel + int2(x, y), int2(0, 0), v2Size - 1);

			if (targetTexture.Load(int3(v2Sample, 0)).a == 0.0)
				return false;
		}
	}

	return true;
}

float4 main(float4 position : POS_OUT) : TAR_OUT
{
    float4 uiColor = uiTexture.Sample(s0, position.xy / viewport.xy);
    if (uiColor.a == 1.0) return lerp(float4(0.0, 0.0, 0.0, 1.0), uiColor, sceneFader);

    float4 debugColor = debugTexture.Sample(s0, position.xy / viewport.xy);
	float4 rawScene = targetTexture.Sample(s0, position.xy / viewport.xy);
	float4 sceneColor;

	if (rawScene.a == 0.0)
	{
		sceneColor = GetBackground(position.xy);
	}
	else if (!IsSceneNeighbourhoodOpaque(position.xy))
	{
		sceneColor = rawScene;
	}
	else
	{
#ifndef __PSSL__
		FxaaTex inputFXAATex = { s0, targetTexture };

		sceneColor = FxaaPixelShader(
			position.xy / viewport.xy,						// FxaaFloat2 pos,
			FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),				// FxaaFloat4 fxaaConsolePosPos,
			inputFXAATex,									// FxaaTex tex,
			inputFXAATex,									// FxaaTex fxaaConsole360TexExpBiasNegOne,
			inputFXAATex,									// FxaaTex fxaaConsole360TexExpBiasNegTwo,
			FxaaFloat2(1.0 / viewport.x, 1.0 / viewport.y),	// FxaaFloat2 fxaaQualityRcpFrame,
			FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),				// FxaaFloat4 fxaaConsoleRcpFrameOpt,
			FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),				// FxaaFloat4 fxaaConsoleRcpFrameOpt2,
			FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),				// FxaaFloat4 fxaaConsole360RcpFrameOpt2,
			0.75f,											// FxaaFloat fxaaQualitySubpix,
			0.166f,											// FxaaFloat fxaaQualityEdgeThreshold,
			0.0833f,										// FxaaFloat fxaaQualityEdgeThresholdMin,
			8.0f,											// FxaaFloat fxaaConsoleEdgeSharpness,
			0.125f,											// FxaaFloat fxaaConsoleEdgeThreshold,
			0.05f,											// FxaaFloat fxaaConsoleEdgeThresholdMin,
			FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f)				// FxaaFloat fxaaConsole360ConstDir,
		);
#else
		sceneColor = targetTexture.Sample(s0, position.xy / viewport.xy);
#endif
	}

    float debugAlpha = debugColor.a;
    debugColor.a = 1.0;

    float uiAlpha = uiColor.a;
    uiColor.a = 1.0;

    float4 outColor = (debugAlpha != 0.0) ? lerp(lerp(sceneColor, debugColor, debugAlpha * 0.5), uiColor, uiAlpha) : lerp(sceneColor, uiColor, uiAlpha);
    return lerp(float4(0.0, 0.0, 0.0, 1.0), outColor, sceneFader);
}