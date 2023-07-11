#pragma once

#include "Core/ECS/Component.h"

#include <External/rttr/type>

#include "Core/Math.h"
#include "Core/VColors.h"

#include <string>
#include "Core/Platform/Rendering/RenderAlignment.h"

class RenderSystem;

class TextRenderer : public Component 
{
	friend class RenderSystem;

public:
	TextRenderer(Entity* pEntity);
	~TextRenderer();

	const std::string& GetText() const { return m_Text; };

	void SetText(const std::string& text) {
		m_Text = text;
	};

	const VColor& GetColor() const { return m_Color; }
	void SetColor(const VColor& color) { m_Color = color; }

	RenderAlignment GetAlignment() const { return m_Alignment; }
	void SetAlignment(RenderAlignment alignment) { m_Alignment = alignment; };

	RenderAlignment GetScreenAlignment() const { return m_ScreenAlignment; }
	void SetScreenAlignment(RenderAlignment alignment) { m_ScreenAlignment = alignment; };

	bool ScalesWithScreen() const { return m_bScaleWithScreen; }
	void SetScaleWithScreen(bool bScale) { m_bScaleWithScreen = bScale; }

	bool IsWrapping() const { return m_bWrapText; }
	void SetWrapping(bool bWrap) { m_bWrapText = bWrap; }

	float GetScale();
	void SetScale(float fScale);

private:
	std::string m_Text = "";

	VColor m_Color = VColors::White;
	bool m_bScaleWithScreen = false;

	bool m_bWrapText = true;

	RenderAlignment m_Alignment = RA_CENTERED;
	RenderAlignment m_ScreenAlignment = RA_BOTTOMLEFT;

	RenderSystem* m_pRenderSystem = nullptr;

	RTTR_ENABLE(Component)
};