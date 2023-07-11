#include "pch.h"
#include "TextureReference.h"

#include "Core/Platform/Rendering/RenderContext.h"

TextureReference::~TextureReference()
{
	Free();

	if (Descriptor)
		delete Descriptor;
}

bool TextureReference::Load(const std::string& filePath)
{
	(void)filePath;
	m_pContext->LoadTexture(this);

	/* Texture becomes either valid or nullptr */
	if (TextureView)
	{
		m_bIsLoaded = true;
	}
	
	return TextureView;
}

void TextureReference::Free()
{
	m_pContext->DestroyTexture(this);
}
