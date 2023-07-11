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

class ComputePass
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

		Shader* m_pShader = nullptr;

		std::vector<View*>				m_Textures;
		std::vector<Buffer*>			m_Buffers;
		std::vector<Mapper*>			m_Mappers;
		std::vector<Sampler*>			m_Samplers;

		UVector3						m_ThreadGroup = UVector3(1, 1, 1);
		uint32_t						m_uiBindlessResourceCount = 0;
	};

	ComputePass(PRenderContext* pContext) { m_pContext = pContext; };
	virtual ~ComputePass() {};

	virtual void Compute(PCommandEngine* pEngine) = 0;
	void SetThreadGroup(const UVector3& threadGroup) { m_Data.m_ThreadGroup = threadGroup; }

	const Data& GetData() const { return m_Data; }

protected:
	ComputePass() {}
	virtual void Init(const Data& data) = 0;

	PRenderContext* m_pContext = nullptr;

	Data m_Data;
};