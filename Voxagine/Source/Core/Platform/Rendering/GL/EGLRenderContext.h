#pragma once

#include "Core/Platform/Rendering/GL/GLRenderContext.h"

typedef void* EGLDisplay;
typedef void* EGLSurface;
typedef void* EGLContext;

class EGLRenderContext : public GLRenderContext {
public:
	EGLRenderContext(Platform* pPlatform) : GLRenderContext(pPlatform) {};
	virtual ~EGLRenderContext() = default;

	virtual void Initialize() override {};
	virtual void Deinitialize() override {};

	virtual bool Present() override { return false; };

private:
	EGLDisplay m_Display;
	EGLSurface m_Surface;
	EGLContext m_Context;
};