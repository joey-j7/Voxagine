#include "pch.h"
#include "Core/ECS/Components/TextRenderer.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>

#include "Core/MetaData/PropertyTypeMetaData.h"
#include "Core/ECS/Systems/Rendering/RenderSystem.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<TextRenderer>("TextRenderer")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Text", &TextRenderer::GetText, &TextRenderer::SetText) (RTTR_PUBLIC)
		.property("Color", &TextRenderer::GetColor, &TextRenderer::SetColor) (RTTR_PUBLIC)
		.property("Wrap", &TextRenderer::IsWrapping, &TextRenderer::SetWrapping) (RTTR_PUBLIC)
		.property("Alignment", &TextRenderer::GetAlignment, &TextRenderer::SetAlignment) (RTTR_PUBLIC)
		.property("Screen Alignment", &TextRenderer::GetScreenAlignment, &TextRenderer::SetScreenAlignment) (RTTR_PUBLIC)
		.property("Scale with Screen", &TextRenderer::ScalesWithScreen, &TextRenderer::SetScaleWithScreen) (RTTR_PUBLIC)
		.property("Scale", &TextRenderer::GetScale, &TextRenderer::SetScale) (RTTR_PUBLIC)
	;
}

TextRenderer::TextRenderer(Entity * pEntity)
	: Component(pEntity)
{
}

TextRenderer::~TextRenderer()
{
}

float TextRenderer::GetScale()
{
	Vector3 v3Scale = GetTransform()->GetScale();
	return std::min(std::min(v3Scale.x, v3Scale.y), v3Scale.z);
}

void TextRenderer::SetScale(float fScale)
{
	GetTransform()->SetScale(Vector3(fScale, fScale, fScale));
}
