#pragma once

#include "ImContext.h"
#include <External/glad/glad.h>

class GLRenderContext;

class GLImContext : public ImContext {
public:
	GLImContext(GLRenderContext* pContext);

	virtual void NewFrame() override;
	virtual void Draw(ImDrawData* drawData) override;

	virtual void Deinitialize() override;

protected:
	void Initialize();

	GLRenderContext* m_pContext;

	// OpenGL Data
	char         m_GlslVersionString[32] = "";
	GLuint       m_FontTexture = 0;
	GLuint       m_ShaderHandle = 0, m_VertHandle = 0, m_FragHandle = 0;
	int          m_AttribLocationTex = 0, m_AttribLocationProjMtx = 0;
	int          m_AttribLocationPosition = 0, m_AttribLocationUV = 0, m_AttribLocationColor = 0;
	unsigned int m_VboHandle = 0, m_ElementsHandle = 0;
};