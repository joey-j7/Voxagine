#include "Defines.hlsl"
#include "CameraData.hlsl" // register(b0)
#include "Camera.hlsl"

SamplerState s0 : register(s0);
Texture2D<float4> targetTexture : register(t0);
Texture2D<float4> uiTexture : register(t1);

VOXEL_RW_BUFFER voxelWorldData : register(u0);

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

/* Sky and endless ground for pixels the Voxel pass left untouched - either no
   AABB proxy covered them, or MarchDiffuse found nothing (see
   VoxelRenderer.ps.hlsl). This pass runs full-screen unconditionally, so
   unlike the proxy-box approach it always covers every pixel. */
float4 GetSkyOrGround(float2 v2ScreenPosition)
{
	float3 rayOrigin = camPosition.xyz - camOffset.xyz;
	float3 rayDirection = normalize(GetRay(mv, v2ScreenPosition, viewport.xy, viewport.z, viewport.w));

	if (rayOrigin.y >= 0.0 && rayDirection.y < 0.0)
	{
		float t = (-1.0 * rayOrigin.y) / rayDirection.y;
		float3 realEndPos = float3(rayOrigin.x + rayDirection.x * t, 0.0, rayOrigin.z + rayDirection.z * t);

		float3 groundPos;
		groundPos.x = abs(fmod(realEndPos.x, float(worldSize.x)));
		groundPos.y = 0.0;
		groundPos.z = abs(fmod(realEndPos.z, float(worldSize.z)));

		float4 groundColor = PostFxGetVoxel(groundPos) * 0.6;
		groundColor.a = 1.0;

		return groundColor;
	}

	return SKY_COLOR;
}

float4 main(float4 position : POS_OUT) : TAR_OUT
{
    float4 uiColor = uiTexture.Sample(s0, position.xy / viewport.xy);
    if (uiColor.a == 1.0) return lerp(float4(0.0, 0.0, 0.0, 1.0), uiColor, sceneFader);

	float4 rawScene = targetTexture.Sample(s0, position.xy / viewport.xy);
	float4 sceneColor;

	if (rawScene.a == 0.0)
	{
		sceneColor = GetSkyOrGround(position.xy);
	}
	else
	{
#ifndef __PSSL__
		FxaaTex inputFXAATex = { s0, targetTexture };

		sceneColor =  FxaaPixelShader(
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

    float uiAlpha = uiColor.a;
    uiColor.a = 1.0;

	return lerp(float4(0.0, 0.0, 0.0, 1.0), lerp(sceneColor, uiColor, uiAlpha), sceneFader);
}