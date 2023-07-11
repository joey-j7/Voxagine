#include "pch.h"
//#include "EGLRenderContext.h"
//
//#include "../../Platform.h"
//#include <Core/Application.h>
//#include "../../Window/WindowContext.h"
//#include "GLHelper.h"
//
//#include <External/glad/glad.h>
//#include "External/EGL/egl.h"
//
//EGLRenderContext::EGLRenderContext(Platform* pPlatform) : GLRenderContext(pPlatform)
//{
//
//}
//
//void EGLRenderContext::Initialize()
//{
//	const Settings& settings = m_pPlatform->GetApplication()->GetSettings();
//	RenderingAPI apiType = settings.GetRenderAPIType();
//
//	const HWND& handle = *(HWND*)m_pPlatform->GetWindowContext()->GetHandle();
//	HDC hdc = GetDC(handle);
//
//	// Create EGL display connection
//	m_Display = eglGetDisplay(hdc);
//
//	// Initialize EGL for this display, returns EGL version
//	EGLint eglVersionMajor = 0, eglVersionMinor = 0;
//	EGL_CHECK(eglInitialize(m_Display, &eglVersionMajor, &eglVersionMinor));
//
//	uint32_t api = apiType == RA_OPENGLES ? EGL_OPENGL_ES_API : EGL_OPENGL_API;
//	EGL_CHECK(eglBindAPI(api));
//
//	EGLint apiBitType = apiType == RA_OPENGLES ? EGL_OPENGL_ES2_BIT : EGL_OPENGL_BIT;
//	EGLint configAttributes[] =
//	{
//		EGL_RENDERABLE_TYPE, apiBitType,
//		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
//		EGL_BLUE_SIZE, 8,
//		EGL_GREEN_SIZE, 8,
//		EGL_RED_SIZE, 8,
//		EGL_DEPTH_SIZE, 0,
//		EGL_NONE
//	};
//
//	EGLint numConfigs = 0;
//	EGLConfig windowConfig;
//	EGL_CHECK(eglChooseConfig(m_Display, configAttributes, &windowConfig, 1, &numConfigs));
//
//	EGLint surfaceAttributes[] = { EGL_NONE };
//	m_Surface = eglCreateWindowSurface(m_Display, windowConfig, handle, surfaceAttributes);
//
//	if (m_Surface == EGL_NO_SURFACE)
//	{
//		EGL_CHECK(0);
//		assert(false);
//	}
//
//	EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
//	m_Context = eglCreateContext(m_Display, windowConfig, NULL, contextAttributes);
//
//	if (eglMakeCurrent(m_Display, m_Surface, m_Surface, m_Context) == EGL_FALSE) {
//		assert(false);
//	}
//
//	if (apiType == RA_OPENGL ? !gladLoadGLLoader((GLADloadproc)eglGetProcAddress) : !gladLoadGLES2Loader((GLADloadproc)eglGetProcAddress))
//		return;
//
//	eglSwapInterval(m_Display, m_pPlatform->GetApplication()->GetSettings().IsVSyncEnabled());
//
//	EGL_CHECK(0);
//
//	GLRenderContext::Initialize();
//}
//
//void EGLRenderContext::Deinitialize()
//{
//	if (m_Display != EGL_NO_DISPLAY) {
//		eglMakeCurrent(m_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
//
//		if (m_Context != EGL_NO_CONTEXT) {
//			eglDestroyContext(m_Display, m_Context);
//		}
//
//		if (m_Surface != EGL_NO_SURFACE) {
//			eglDestroySurface(m_Display, m_Surface);
//		}
//		eglTerminate(m_Display);
//	}
//
//	m_Display = EGL_NO_DISPLAY;
//	m_Surface = EGL_NO_SURFACE;
//
//	GLRenderContext::Deinitialize();
//}
//
//void EGLRenderContext::Present()
//{
//	GLRenderContext::Present();
//
//	eglSwapBuffers(m_Display, m_Surface);
//	EGL_CHECK(0);
//}
