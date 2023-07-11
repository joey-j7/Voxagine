#include "Defines.hlsl"

float4 main(uint uiID : VERT_ID) : POS_OUT
{
    float2 v2TexCoord = float2((uiID << 1) & 2, uiID & 2);
    return float4(v2TexCoord.x * 2 - 1, -v2TexCoord.y * 2 + 1, 0, 1);
}