#pragma once

#include "Core/Platform/Rendering/Managers/TextureManager.h"
#include "Core/Resources/Formats/TextureReference.h"

#include <stdint.h>
#include <unordered_map>

class DXHeapManager;
class DX12RenderContext;
class CommandEngine;

class DXTextureManager : public TextureManager
{
public:
	DXTextureManager(DX12RenderContext* pContext, DXHeapManager* pManager = nullptr);
	virtual ~DXTextureManager();

	virtual uint32_t CreateTexture(CommandEngine* pEngine, const std::string& sName, uint8_t* pData, UVector2 uSize) override;
	virtual uint32_t CreateEmptyTexture() override;

	virtual bool LoadTexture(CommandEngine* pEngine, TextureReference* pTextureReference) override;
	virtual void DestroyTexture(const TextureReference* pTextureReference) override;

	DXHeapManager* GetHeapManager() const { return m_pHeapManager; }

private:
	bool m_bIsSRVOwner = false;
	DXHeapManager* m_pHeapManager = nullptr;
};