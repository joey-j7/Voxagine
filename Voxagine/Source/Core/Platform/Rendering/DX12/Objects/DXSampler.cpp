#include "pch.h"

#include "Core/Platform/Rendering/Objects/Sampler.h"

Sampler::Sampler(PRenderContext* pContext, const Info& info)
{
	m_pContext = pContext;
	m_Info = info;
};