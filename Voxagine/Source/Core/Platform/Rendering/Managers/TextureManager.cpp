#include "pch.h"
#include "TextureManager.h"

#include "Core/Platform/Rendering/RenderDefines.h"
#include "Core/Platform/Rendering/RenderContextInc.h"
#include "Core/Platform/Platform.h"
#include "Core/Application.h"
#include "Core/LoggingSystem/LoggingSystem.h"

#include "Core/Platform/Rendering/RenderContextInc.h"
#include "Core/Platform/Rendering/CommandEngineInc.h"
#include "Core/Platform/Rendering/RenderDefines.h"

#include "Core/Resources/Formats/TextureReference.h"

#include <External/STB/stb_image_aug.h>

TextureManager::TextureManager(PRenderContext* pContext) { m_pContext = pContext; }

TextureReadData* TextureManager::ReadTexture(const std::string& texturePath)
{
	TextureReadData* data = new TextureReadData();

	/*	try to load the image	*/
	FileSystem* pFileSystem = m_pContext->GetPlatform()->GetApplication()->GetFileSystem();
	FH handle = pFileSystem->OpenFile(texturePath.c_str(), FSOF_READ | FSOF_BINARY);

	if (!handle)
	{
		LoggingSystem& logger = m_pContext->GetPlatform()->GetApplication()->GetLoggingSystem();
		logger.Log(LOGLEVEL_ERROR, "RenderContext", "Failed to load texture with path: " + texturePath);

		return data;
	}

	FSize fileSize = pFileSystem->GetFileSize(handle);

	uint8_t* pBuffer = new uint8_t[fileSize];
	pFileSystem->Read(handle, pBuffer, 1, fileSize);
	pFileSystem->CloseFile(handle);

	int width, height, channels;
	uint8_t* img = stbi_load_from_memory(pBuffer, fileSize, &width, &height, &channels, 4);

	delete[] pBuffer;

	if (!img)
	{
		LoggingSystem& logger = m_pContext->GetPlatform()->GetApplication()->GetLoggingSystem();
		logger.Log(LOGLEVEL_ERROR, "RenderContext", "Failed to parse texture with path: " + texturePath);

		return data;
	}

	data->m_Data = reinterpret_cast<uint32_t*>(img);
	data->m_Dimensions.x = width;
	data->m_Dimensions.y = height;

	return data;
}

void TextureManager::DestroyTexture(const TextureReference* pTextureReference)
{
	if (pTextureReference)
	{
		auto it = m_pViews.find(pTextureReference->GetID());

		if (it != m_pViews.end())
		{
			it->second.reset();
			m_pViews.erase(it);
		}
	}
}

PTextureManager* TextureManager::Get()
{
	return reinterpret_cast<PTextureManager*>(this);
}