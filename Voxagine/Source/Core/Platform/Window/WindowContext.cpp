#include "pch.h"
#include "WindowContext.h"

#include "Core/Application.h"
#include "Core/Platform/Platform.h"

#include "Core/Platform/Rendering/RenderContext.h"

#include <stdint.h>

WindowContext::WindowContext(Platform* pPlatform)
{
	m_pPlatform = pPlatform;
}

WindowContext::~WindowContext()
{
}

void WindowContext::Initialize()
{
	RenderContext* pRenderContext = m_pPlatform->GetRenderContext();

	pRenderContext->FullscreenChanged += Event<bool>::Subscriber(std::bind(&WindowContext::OnFullscreenChanged, this, std::placeholders::_1), this);
	pRenderContext->SizeChanged += Event<uint32_t, uint32_t, IVector2>::Subscriber(std::bind(&WindowContext::OnResize, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), this);
}

const UVector2& WindowContext::GetPosition() const
{
	return m_v2Position;
}

const UVector2& WindowContext::GetSize() const
{
	return m_v2Size;
}