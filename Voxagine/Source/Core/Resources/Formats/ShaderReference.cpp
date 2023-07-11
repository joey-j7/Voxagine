#include "pch.h"
#include "ShaderReference.h"

#include "Core/Platform/Rendering/RenderContext.h"

ShaderReference::~ShaderReference()
{
	Free();
}

bool ShaderReference::Load(const std::string& filePath)
{
	(void)filePath;
	m_pContext->LoadShader(this);

	/* Texture becomes either valid or nullptr */
	if (Shader)
	{
		m_bIsLoaded = true;
	}

	return Shader;
}

void ShaderReference::Free()
{
	m_pContext->DestroyShader(this);
}
