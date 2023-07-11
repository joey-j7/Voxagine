#pragma once

#include <corecrt_wstdio.h>
#include <stdio.h>
#include <string>

#ifdef _DEBUG
#define GL_CHECK(x) \
    x; \
    { \
        GLint glError = glGetError(); \
        if(glError != GL_NO_ERROR) { \
			char buf[256]; \
			sprintf_s(buf, sizeof(buf), "glGetError() = %i (0x%.8x) at line %i\n", glError, glError, __LINE__); \
			OutputDebugString(buf); \
			__debugbreak(); \
        } \
    }

#define EGL_CHECK(x) \
    x; \
    { \
        EGLint eglError = eglGetError(); \
        if(eglError != EGL_SUCCESS) { \
			char buf[256]; \
			sprintf_s(buf, sizeof(buf), "eglGetError() = %i (0x%.8x) at line %i\n", eglError, eglError, __LINE__); \
			OutputDebugString(buf); \
			__debugbreak(); \
        } \
    }
#else
#define GL_CHECK(x) x; {}
#define EGL_CHECK(x) x; {}
#endif