#pragma once

#include <vector>
#include <memory>

#include <string>
#include <stdint.h>
#include <unordered_map>

#include "Core/Platform/Rendering/RenderDefines.h"
#include "Core/Platform/Rendering/Objects/View.h"

class Shader;
class Sampler;
class Buffer;
class Mapper;

class RenderPass
{
	friend class Shader;
	friend class Sampler;
	friend class Buffer;
	friend class Mapper;
	friend class View;

public:
	struct Data
	{
		std::string						m_Name = "Unnamed";

		PPrimitiveTopology				m_Topology = R_DEF_PRIMITIVE_TOPOLOGY;
		PPrimitiveTopologyType			m_TopologyType = R_DEF_PRIMITIVE_TOPOLOGY_TYPE;

		PResourceStates					m_TargetType = R_DEF_RESOURCE_STATE_TYPE;
		std::vector<PResourceFormat>	m_TargetFormat = { R_DEF_RESOURCE_FORMAT };

		Shader*							m_pVertexShader = nullptr;
		Shader*							m_pPixelShader = nullptr;

		std::vector<View*>				m_Textures;
		std::vector<Buffer*>			m_Buffers;
		std::vector<Mapper*>			m_Mappers;
		std::vector<Sampler*>			m_Samplers;
		std::vector<PRenderPass*>		m_PassOutput;

		uint32_t						m_uiRenderViewCount = 1;
		uint32_t						m_uiInstanceCount = 1;

		uint32_t						m_uiVertexCount = 0;
		void*							m_pVertexData = nullptr;

		PVertexLayout					m_VertexLayout;

		uint32_t						m_uiIndexCount = 0;
		void*							m_pIndexData = nullptr;

		bool							m_bClearPerFrame = true;
		float							m_DepthClearValue = 1.0f;
		Vector4							m_ClearColor = Vector4(0.f, 0.f, 0.f, 0.f);

		bool							m_bEnableDepth = false;
		PResourceFormat					m_DepthFormat = R_DEF_DEPTH_FORMAT;

		PCullMode						m_CullType = R_DEF_CULL_TYPE;

		uint32_t						m_uiBindlessResourceCount = 0;

		Vector2							m_TargetSize = Vector2(0.f, 0.f);
		bool							m_bUseScreenResolution = true;
		float							m_fRenderScale = 1.0f;

		bool							m_BlendEnabled = false;
		uint32_t						m_uiBackBuffers = 0;
	};

	RenderPass(PRenderContext* pContext) { m_pContext = pContext; };
	virtual ~RenderPass() {};

	virtual void Begin(PCommandEngine* pEngine) = 0;
	virtual void Draw(PCommandEngine* pEngine) = 0;
	virtual void End(PCommandEngine* pEngine) = 0;

	virtual void Clear(PCommandEngine* pEngine) = 0;

	virtual View* GetTargetView(uint32_t i) const = 0;
	virtual View* GetDepthView() const = 0;

	virtual void Resize(UVector2 uSize) = 0;

	virtual UVector2 GetTargetSize() const = 0;
	const Data& GetData() const { return m_Data; }

	void ToggleBackBuffer() { m_uiCurrentBackBuffer = (m_uiCurrentBackBuffer + 1) % (m_Data.m_uiBackBuffers + 1); }
	uint32_t GetPreviousBackBuffer() const { return m_uiCurrentBackBuffer == 0 ? m_Data.m_uiBackBuffers : m_uiCurrentBackBuffer - 1; }

protected:
	RenderPass() {}
	virtual void Init(const Data& data) = 0;

	PRenderContext* m_pContext = nullptr;

	uint32_t m_uiCurrentBackBuffer = 0;

	Data m_Data;
	std::vector<std::unique_ptr<View>> m_pTargetViews;
	std::unique_ptr<View> m_pDepthView;

	float m_fInvRenderScale = 1.0f;

	bool m_bIsDrawn = false;
	bool m_bIsCleared = false;
};