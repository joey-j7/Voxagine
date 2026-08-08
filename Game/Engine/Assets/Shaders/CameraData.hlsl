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

	/* Far-field cell grid (RENDERING_PLAN.md phase 4), or zero when there is no
	   far field to march - see RenderContext::GetFarFieldShaderGridSize.

	   Appended, never inserted: RenderContext::Present fills this buffer field
	   by field in declaration order, so anything added above shifts everything
	   after it. The four scalars above pack into one 16-byte register, so this
	   lands aligned. */
	uint4 farFieldSize;

	/* The camera the *scene* was rendered with, which is not the camera in the
	   fields above.
	   RenderContext::Present copies the voxel pass's target at the top of the
	   frame and post processing samples that copy - so the image it composites
	   was rendered one submission ago, while camPosition above has already
	   moved on to the render being recorded now. Anything in post processing
	   that reconstructs a world-space ray has to use these instead, or it
	   slides against the image underneath it whenever the camera moves.
	   Sky and ground never noticed, because they read only the ray's Y.
	   Appended, never inserted - see farFieldSize. */
	matrix sceneInvMvp;

	float4 sceneCamPosition;
	float4 sceneCamOffset;
};