#pragma once

#include "Core/Platform/Rendering/RenderDefines.h"

class Sampler
{
public:
	struct Info
	{
		PFilterMode			m_FilterMode = R_DEF_FILTER_MODE;
		PWrapMode			m_WrapMode = R_DEF_WRAP_MODE;
	};

	Sampler(PRenderContext* pContext, const Info& info);
	virtual ~Sampler() { delete m_pNativeSampler; }

	const Info& GetInfo() const { return m_Info; }
	void*& GetNative() { return m_pNativeSampler; }

	bool IsInitialized() const { return m_bInitialized; }

protected:
	PRenderContext* m_pContext = nullptr;
	Info m_Info;

	void* m_pNativeSampler = nullptr;
	bool m_bInitialized = false;
};