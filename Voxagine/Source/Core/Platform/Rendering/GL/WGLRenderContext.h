/* WGL Context that relies on opengl32.lib */

#ifdef _UNDEFINED_
#pragma once

#include "Core/Platform/Rendering/GL/GLRenderContext.h"

class WGLRenderContext : public GLRenderContext {
public:
	WGLRenderContext(Platform* pPlatform);
	virtual ~WGLRenderContext() = default;

	virtual void Initialize() override;
	virtual void Deinitialize() override;

	virtual void Present() override;

	virtual void UploadVoxels(uint32_t uiViewStart, uint32_t uiViewEnd, void* pSourceStart, void* pSourceEnd) override;

	bool SetSwapInterval(int iInterval);

private:
	HDC m_Hdc;
	HGLRC m_Context;
};
#endif