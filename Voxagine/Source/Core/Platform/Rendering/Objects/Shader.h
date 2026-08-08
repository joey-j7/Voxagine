#pragma once

#include <string>
#include <memory>

#include "Core/Platform/Rendering/RenderDefines.h"

class Shader
{
public:
	enum Type
	{
		E_VERTEX,
		E_PIXEL,
		E_COMPUTE
	};

	struct Info
	{
		std::string		m_FilePath = "";
		Type			m_Type = E_VERTEX;
	};

	Shader(PRenderContext* pContext, const Info& info);
	~Shader();

	const Info& GetInfo() const { return m_Info; }
	uint32_t GetHandle() const { return m_uiHandle; }

	const PShaderCache& GetCache() const { return m_Cache; }

	PShader* GetNative() const;

protected:
	PRenderContext* m_pContext = nullptr;
	Info m_Info;

	uint32_t m_uiHandle = UINT_MAX;

	R_PTR_TYPE(PShader) m_pNativeShader = nullptr;

	PShaderCache m_Cache;
};