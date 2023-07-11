#pragma once

#include <string>
#include <stdint.h>
#include <memory>

#include "Core/Math.h"
#include "Core/Platform/Rendering/RenderDefines.h"

class RenderPass;

class View
{
	friend class RenderPass;

	friend class DXRenderPass;
	friend class ORBRenderPass;
	friend class GLRenderPass;

public:
	enum Type
	{
		E_RENDER_TARGET_VIEW,
		E_DEPTH_STENCIL_VIEW,
		E_SHADER_RESOURCE_VIEW
	};

	struct TargetData
	{
		PRenderPass*				m_pTarget = nullptr;
		uint32_t					m_uiID;

#ifdef _ORBIS
		std::unique_ptr<PTexture>	m_pTexture = nullptr;
#endif

		Type						m_Type;
	};

	struct Info
	{
		PResourceDimension			m_DimensionType = R_DEF_RESOURCE_DIMENSION_TYPE;
		UVector3					m_Size = UVector3(0.f, 0.f, 0.f);
		PResourceFormat				m_ColorFormat = R_DEF_RESOURCE_FORMAT;
		PResourceStates				m_State = R_DEF_RESOURCE_STATE_TYPE;
		Type						m_Type = E_SHADER_RESOURCE_VIEW;

		std::string					m_Name = "Unnamed";
	};

	View(PRenderContext* pContext)
	{
		m_pContext = pContext;
	}

	View(PRenderContext* pContext, const Info& info)
	{
		m_pContext = pContext;
		m_Info = info;

		// This also creates the resource
		Resize(m_Info.m_Size);
	}

	~View();

	const Info& GetInfo() const { return m_Info; }
	uint8_t* GetData() const { return m_pData; }

	PResourceStates SetState(PCommandEngine* pEngine, PResourceStates state);
	void Resize(const UVector3& uSize);

	void AddTarget(PRenderPass* pRenderPass, uint32_t uiID, Type type);
	PTextureType*& GetNative() { return m_pNativeTexture; }

#ifdef _ORBIS
	PTextureType*& GetNativeTarget() { return m_pNativeTarget; }
#endif

protected:
	void CreateTarget(TargetData& data);

	PRenderContext* m_pContext = nullptr;

	Info m_Info;

	uint8_t* m_pData = nullptr;

	std::vector<TargetData> m_TargetData;
	PTextureType* m_pNativeTexture = nullptr;

#ifdef _ORBIS
	PTextureType* m_pNativeTarget = nullptr;
#endif
};