#include "pch.h"
#include "Core/ECS/Components/SpriteRenderer.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>
#include "Core/ECS/Systems/Rendering/RenderSystem.h"
#include "Core/Application.h"

#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	/*
	 * @brief Example enumeration registration with RTTR
	 * @param Class, Registration name
	 * @param <name, EnumValue>
	 */
	rttr::registration::enumeration<RenderAlignment>("RenderAlignment")
	(
		rttr::value("Centered",		RenderAlignment::RA_CENTERED),
		rttr::value("Top Left",		RenderAlignment::RA_TOPLEFT),
		rttr::value("Top Center",	RenderAlignment::RA_TOPCENTER),
		rttr::value("Top Right",	RenderAlignment::RA_TOPRIGHT),
		rttr::value("Right Center",	RenderAlignment::RA_RIGHTCENTER),
		rttr::value("Left Center",	RenderAlignment::RA_LEFTCENTER),
		rttr::value("Bottom Left",  RenderAlignment::RA_BOTTOMLEFT),
		rttr::value("Bottom Center",RenderAlignment::RA_BOTTOMCENTER),
		rttr::value("Bottom Right",	RenderAlignment::RA_BOTTOMRIGHT)
	);

	rttr::registration::class_<SpriteRenderer>("SpriteRenderer")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("File", &SpriteRenderer::GetFilePath, &SpriteRenderer::SetFilePath) ( RTTR_PUBLIC, RTTR_RESOURCE("png") )
		.property("Color", &SpriteRenderer::GetColor, &SpriteRenderer::SetColor) (RTTR_PUBLIC)
		.property("Alignment", &SpriteRenderer::GetAlignment, &SpriteRenderer::SetAlignment) (RTTR_PUBLIC)
		.property("Screen Alignment", &SpriteRenderer::GetScreenAlignment, &SpriteRenderer::SetScreenAlignment) (RTTR_PUBLIC)
		.property("Use Screen Space", &SpriteRenderer::IsScreenSpace, &SpriteRenderer::SetToScreenSpace) (RTTR_PUBLIC)
		.property("Scale with Screen", &SpriteRenderer::ScalesWithScreen, &SpriteRenderer::SetScaleWithScreen) (RTTR_PUBLIC)
		.property("Enable Billboard", &SpriteRenderer::IsBillboard, &SpriteRenderer::SetBillboard) (RTTR_PUBLIC)
		.property("Render Layer", &SpriteRenderer::GetRenderLayer, &SpriteRenderer::SetRenderLayer) (RTTR_PUBLIC)
		.property("Tiling", &SpriteRenderer::GetTilingAmount, &SpriteRenderer::SetTilingAmount) (RTTR_PUBLIC)
		.property("Culling Start", &SpriteRenderer::GetCullingStart, &SpriteRenderer::SetCullingStart) (RTTR_PUBLIC)
		.property("Culling End", &SpriteRenderer::GetCullingEnd, &SpriteRenderer::SetCullingEnd) (RTTR_PUBLIC)
	;
}

SpriteRenderer::SpriteRenderer(Entity * pEntity)
	: Component(pEntity)
{

}

SpriteRenderer::~SpriteRenderer()
{
	if (m_pTextureReference)
	{
		m_pTextureReference->Release();
		m_pTextureReference = nullptr;
	}
}

void SpriteRenderer::SetFilePath(const std::string& filepath)
{
	if (m_FilePath == filepath)
		return;

	m_UnloadFilePath = m_FilePath;
	m_FilePath = filepath;

	if (m_pTextureReference)
	{
		m_pTextureReference->Release();
		m_pTextureReference = nullptr;
	}

	if (!filepath.empty())
	{
		m_pTextureReference = GetWorld()->GetApplication()->GetResourceManager().LoadTexture(filepath);
		m_UnloadFilePath = filepath;
	}
}

UVector2 SpriteRenderer::GetSize(bool bUseScale) const
{
	Vector2 size = m_pTextureReference->TextureView->GetInfo().m_Size;

	if (bUseScale)
	{
		Vector2 scale = GetScale();
		size *= std::min(scale.x, scale.y);
	}

	return size;
}

Vector2 SpriteRenderer::GetScale() const
{
	if (IsScreenSpace() && ScalesWithScreen())
	{
		Vector2 renderRes = m_pRenderSystem->GetRenderResolution();
		return Vector2(renderRes.x / 1280.f, renderRes.y / 720.f);
	}

	return Vector2(1.f, 1.f);
}
