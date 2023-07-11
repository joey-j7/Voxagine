#pragma once

#include "Core/Resources/ReferenceObject.h"
#include "Core/Platform/Rendering/Objects/View.h"

#include <string>

class RenderContext;

class TextureReference : public ReferenceObject {
	friend class DXTextureManager;
	friend class ORBTextureManager;

public:
	friend class TextureManager;

	TextureReference(const std::string& filePath) : ReferenceObject(filePath) {};
	virtual ~TextureReference();

	virtual bool Load(const std::string& filePath) override;
	virtual void Free() override;

	void SetContext(RenderContext* pContext) { m_pContext = pContext; };
	uint32_t GetID() const { return m_uiID; }

	View* TextureView = nullptr;
	void* Descriptor = nullptr;

private:
	RenderContext* m_pContext = nullptr;

	uint32_t m_uiID = 0;
};