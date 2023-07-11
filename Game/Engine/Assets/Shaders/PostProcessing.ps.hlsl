#include "Defines.hlsl"
#include "CameraData.hlsl" // register(b0)

SamplerState s0;
Texture2D<float4> targetTexture : register(t0);
Texture2D<float4> uiTexture : register(t1);

#ifndef __PSSL__
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 39
#include "FXAA3_11.hlsl"
#endif

float4 main(float4 position : POS_OUT) : TAR_OUT
{
    float4 uiColor = uiTexture.Sample(s0, position.xy / viewport.xy);
    if (uiColor.a == 1.0) return lerp(float4(0.0, 0.0, 0.0, 1.0), uiColor, sceneFader);
	
	float4 sceneColor;
	
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

    float uiAlpha = uiColor.a;
    uiColor.a = 1.0;

	return lerp(float4(0.0, 0.0, 0.0, 1.0), lerp(sceneColor, uiColor, uiAlpha), sceneFader);
}