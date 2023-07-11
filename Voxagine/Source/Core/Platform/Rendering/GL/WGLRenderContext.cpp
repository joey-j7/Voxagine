/* WGL Context that relies on opengl32.lib */

#include "pch.h"

#ifdef _UNDEFINED_

#include "WGLRenderContext.h"

#include <windows.h>
#include <External/glad/glad.h>

#include "../../Platform.h"

#include <Core/Application.h>
#include "../../Window/WindowContext.h"
#include "GLHelper.h"

GLAPI int GLAD_WGL_EXT_swap_control;
typedef BOOL(APIENTRYP PFNWGLSWAPINTERVALEXTPROC)(int interval);
GLAPI PFNWGLSWAPINTERVALEXTPROC glad_wglSwapIntervalEXT = 0;
#define wglSwapIntervalEXT glad_wglSwapIntervalEXT
typedef int (APIENTRYP PFNWGLGETSWAPINTERVALEXTPROC)(void);
GLAPI PFNWGLGETSWAPINTERVALEXTPROC glad_wglGetSwapIntervalEXT = 0;
#define wglGetSwapIntervalEXT glad_wglGetSwapIntervalEXT

WGLRenderContext::WGLRenderContext(Platform* pPlatform) : GLRenderContext(pPlatform)
{
}

void WGLRenderContext::Initialize()
{
	const Settings& settings = m_pPlatform->GetApplication()->GetSettings();
	RenderingAPI apiType = settings.GetRenderAPIType();

	const HWND& handle = *(HWND*)m_pPlatform->GetWindowContext()->GetHandle();
	m_Hdc = GetDC(handle);

	/* Pixel format */
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,
		32,             // zbuffer
		0,              // stencil!
		0,
		PFD_MAIN_PLANE,
		0, 0, 0, 0
	};

	int iPixelFormat = ChoosePixelFormat(m_Hdc, &pfd);
	SetPixelFormat(m_Hdc, iPixelFormat, &pfd);

	/* WGL Context */
	m_Context = wglCreateContext(m_Hdc);
	wglMakeCurrent(m_Hdc, m_Context);

	if (!gladLoadGL())
		return;

	SetSwapInterval(m_pPlatform->GetApplication()->GetSettings().IsVSyncEnabled());

	GLRenderContext::Initialize();
}

void WGLRenderContext::Deinitialize()
{
	if (m_Context) {
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(m_Context);
	}

	ReleaseDC(*(HWND*)m_pPlatform->GetWindowContext()->GetHandle(), m_Hdc);

	GLRenderContext::Deinitialize();
}

void WGLRenderContext::Present()
{
	GLRenderContext::Present();

	SwapBuffers(m_Hdc);
	GL_CHECK(0);
}

bool WGLRenderContext::SetSwapInterval(int iInterval) {
	// Extension is supported, init pointers.
	glad_wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

	// this is another function from WGL_EXT_swap_control extension
	glad_wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

	glad_wglSwapIntervalEXT(iInterval);

	return true;
}
#endif