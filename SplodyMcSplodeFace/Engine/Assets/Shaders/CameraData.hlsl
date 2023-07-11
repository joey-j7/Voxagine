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