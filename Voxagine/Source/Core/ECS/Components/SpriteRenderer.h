#pragma once

#include "Core/ECS/Component.h"

#include <External/rttr/type>

#include "Core/Math.h"
#include "Core/VColors.h"

#include "Core/Platform/Rendering/RenderAlignment.h"
#include "Core/Resources/Formats/TextureReference.h"

class RenderSystem;

class SpriteRenderer : public Component 
{
	friend class RenderSystem;

public:
	SpriteRenderer(Entity* pEntity);
	~SpriteRenderer();

	const VColor& GetColor() const { return m_Color; }
	void SetColor(const VColor& color) { m_Color = color; }

	const std::string& GetFilePath() const { return m_FilePath; }
	void SetFilePath(const std::string& filepath);

	RenderAlignment GetAlignment() const { return m_Alignment; }
	void SetAlignment(RenderAlignment alignment) { m_Alignment = alignment; };

	RenderAlignment GetScreenAlignment() const { return m_ScreenAlignment; }
	void SetScreenAlignment(RenderAlignment alignment) { m_ScreenAlignment = alignment; };

	bool IsScreenSpace() const { return m_bScreenSpace; }
	void SetToScreenSpace(bool bScreenSpace) { m_bScreenSpace = bScreenSpace; }

	bool ScalesWithScreen() const { return m_bScaleWithScreen; }
	void SetScaleWithScreen(bool bScale) { m_bScaleWithScreen = bScale; }

	bool IsBillboard() const { return m_bIsBillboard; }
	void SetBillboard(bool bBillboard) { m_bIsBillboard = bBillboard; }

	int GetRenderLayer() const { return m_Layer; }
	void SetRenderLayer(int iRenderLayer) { m_Layer = glm::clamp(iRenderLayer, -9999, 9999); }

	Vector2 GetTilingAmount() const { return m_Tiling; }
	void SetTilingAmount(Vector2 tilingAmount) { m_Tiling = tilingAmount; }

	Vector2 GetCullingStart() const { return m_CullStart; }
	void SetCullingStart(Vector2 cullStart) { m_CullStart = cullStart; }
	Vector2 GetCullingEnd() const { return m_CullEnd; }
	void SetCullingEnd(Vector2 cullEnd) { m_CullEnd = cullEnd; }

	UVector2 GetSize(bool bUseScale = true) const;
	Vector2 GetScale() const;

private:
	std::string m_FilePath = "";
	std::string m_UnloadFilePath = "";

	VColor m_Color = VColors::White;

	RenderAlignment m_Alignment = RA_CENTERED;
	RenderAlignment m_ScreenAlignment = RA_BOTTOMLEFT;

	Vector2 m_AlignmentOffset = Vector2(0.f, 0.f);

	bool m_bScreenSpace = false;
	bool m_bScaleWithScreen = false;

	bool m_bIsBillboard = false;

	int m_Layer = 0;

	Vector2 m_Tiling = Vector2(1.f);

	Vector2 m_CullStart = Vector2(0.f);
	Vector2 m_CullEnd = Vector2(1.f);

	RenderSystem* m_pRenderSystem = nullptr;
	TextureReference* m_pTextureReference = nullptr;

	RTTR_ENABLE(Component)
};