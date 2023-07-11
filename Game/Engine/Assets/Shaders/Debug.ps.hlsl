#include "Defines.hlsl"

struct PS_in
{
    float4 Position		: POS_OUT;
    float4 Color		: COLOR;
};

float4 main(PS_in IN) : TAR_OUT
{
	return IN.Color;
}