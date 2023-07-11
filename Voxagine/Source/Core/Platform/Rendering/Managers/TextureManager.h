#pragma once

#include "Core/Platform/Rendering/Objects/View.h"

#include <unordered_map>

class TextureReference;
class LoggingSystem;

class CommandEngine;

struct TextureReadData;

class TextureManager
{
	friend class DXImContext;
	friend class GLImContext;

public:
	TextureManager(PRenderContext* pContext);
	virtual ~TextureManager() = default;

	TextureReadData* ReadTexture(const std::string& texturePath);

	virtual uint32_t CreateTexture(CommandEngine* pEngine, const std::string& sName, uint8_t* pData, UVector2 uSize) = 0;
	virtual uint32_t CreateEmptyTexture() = 0;

	virtual bool LoadTexture(CommandEngine* pEngine, TextureReference* pTextureReference) = 0;
	virtual void DestroyTexture(const TextureReference* pTextureReference);

	PTextureManager* Get();

	PRenderContext* m_pContext = nullptr;
	std::unordered_map<uint32_t, std::unique_ptr<View>> m_pViews;
};