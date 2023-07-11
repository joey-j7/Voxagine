#include "Defines.hlsl"

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

SamplerState s0;

Texture2D<float4> targetTexture : register(t0);
Texture2D<float4> uiTexture : register(t1);
Texture2D<float4> debugTexture : register(t2);

#ifndef __PSSL__
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 39
#include "FXAA3_11.hlsl"
#endif

float4 main(float4 position : POS_OUT) : TAR_OUT
{
    float4 uiColor = uiTexture.Sample(s0, position.xy / viewport.xy);
    if (uiColor.a == 1.0) return uiColor;
	
    float4 debugColor = debugTexture.Sample(s0, position.xy / viewport.xy);
    if (debugColor.a != 0.0) return lerp(debugColor, uiColor, uiColor.a);
	
	float4 sceneColor;
	
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

	return lerp(sceneColor, uiColor, uiColor.a);
}