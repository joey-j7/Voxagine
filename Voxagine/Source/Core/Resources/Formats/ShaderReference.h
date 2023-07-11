#pragma once

#include "Core/Resources/ReferenceObject.h"
#include <string>

class RenderContext;

class ShaderReference : public ReferenceObject {
public:
	ShaderReference(const std::string& filePath) : ReferenceObject(filePath) {};
	virtual ~ShaderReference();

	virtual bool Load(const std::string& filePath) override;
	virtual void Free() override;

	void SetContext(RenderContext* pContext) { m_pContext = pContext; };

	void* Shader = nullptr;

private:
	RenderContext* m_pContext = nullptr;
};