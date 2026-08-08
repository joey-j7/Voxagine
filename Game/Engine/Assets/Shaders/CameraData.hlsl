CONSTANT_BUFFER Data : register(b0)
{
	matrix mvp;
    matrix mv;
	
    float4 camPosition;
    float4 camOffset;
	
	float4 viewport;
    float4 lightDirection;
	
    uint4 worldSize;
	
    float voxelRenderScale;
    float sceneFader;

	uint particleCount;
	uint sdfCount;

	/* Inverse of mvp, for reconstructing a camera ray from a pixel. Appended
	   last on purpose: RenderContext::Present fills this buffer field by field
	   in declaration order, so anything inserted above shifts everything after
	   it. The four scalars above pack into one 16-byte register, so this starts
	   16-byte aligned as a matrix must. */
	matrix invMvp;
};