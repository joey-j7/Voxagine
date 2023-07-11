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

#ifdef _ORBIS
	PPShader* GetNativePS() const;
	PVShader* GetNativeVS() const;

	void* GetData() const { return m_pData; }
#else
	PShader* GetNative() const;
#endif

protected:
	PRenderContext* m_pContext = nullptr;
	Info m_Info;

	uint32_t m_uiHandle = UINT_MAX;

#ifdef _ORBIS
	PVShader* m_pNativeVShader = nullptr;
	PPShader* m_pNativePShader = nullptr;
	void* m_pData = nullptr;
#else
	R_PTR_TYPE(PShader) m_pNativeShader = nullptr;
#endif

	PShaderCache m_Cache;
};